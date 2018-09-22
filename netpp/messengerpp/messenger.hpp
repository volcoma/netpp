#pragma once
#include "messenger.h"

namespace net
{
template <typename T, typename OArchive, typename IArchive>
typename messenger<T, OArchive, IArchive>::ptr messenger<T, OArchive, IArchive>::create()
{
	struct make_shared_enabler : messenger
	{
	};
	return std::make_shared<make_shared_enabler>();
}

template <typename T, typename OArchive, typename IArchive>
connector::id_t
messenger<T, OArchive, IArchive>::add_connector(const connector_ptr& connector, on_connect_t on_connect,
												on_disconnect_t on_disconnect, on_msg_t on_msg)
{
	if(!connector)
	{
		return 0;
	}
	auto connector_id = connector->id;
	if(connector->on_connection_ready)
	{
		return connector_id;
	}
	auto info = std::make_shared<user_info>();
	info->on_connect = std::move(on_connect);
	info->on_disconnect = std::move(on_disconnect);
	info->on_msg = std::move(on_msg);
	info->thread_id = itc::this_thread::get_id();

	auto weak_this = weak_ptr(this->shared_from_this());

	connector->on_connection_ready = [info, weak_this](connection_ptr connection) {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}

		shared_this->on_new_connection(connection, info);
	};

	{
		std::lock_guard<std::mutex> lock(guard_);
		connectors_.emplace(connector->id, connector);
	}
	connector->start();
	return connector_id;
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::send_msg(connection::id_t id, msg_t&& msg)
{
	std::unique_lock<std::mutex> lock(guard_);

	auto conn_it = connections_.find(id);
	if(conn_it == std::end(connections_))
	{
		return;
	}
	auto& conn_info = conn_it->second;

	// get a copy so that we can unlock
	// and work on unlocked mutex
	// after the unlock we may be the last
	// user of this connection.
	auto connection = conn_info.connection;

	lock.unlock();

	connection->send_msg(serializer_t::to_buffer(msg), 0);
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::disconnect(connection::id_t id)
{
	std::unique_lock<std::mutex> lock(guard_);

	auto conn_it = connections_.find(id);
	if(conn_it == std::end(connections_))
	{
		return;
	}
	auto& conn_info = conn_it->second;

	// get a copy so that we can unlock
	// and work on unlocked mutex
	// after the unlock we may be the last
	// user of this connection.
	auto connection = conn_info.connection;

	lock.unlock();

	connection->stop({});
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::remove_connector(connector::id_t id)
{
	std::lock_guard<std::mutex> lock(guard_);
	connectors_.erase(id);
}

template <typename T, typename OArchive, typename IArchive>
bool messenger<T, OArchive, IArchive>::empty() const
{
	std::lock_guard<std::mutex> lock(guard_);
	return connectors_.empty() && connections_.empty();
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::remove_all()
{
	auto connections = [&]() {
		std::lock_guard<std::mutex> lock(guard_);
		connectors_.clear();
		return std::move(connections_);
	}();

	// safely iterate the extracted connections
	for(auto& kvp : connections)
	{
		auto& conn_info = kvp.second;
		auto& connection = conn_info.connection;
		connection->stop({});
		connection.reset();
	}
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::on_new_connection(connection_ptr& connection,
														 const user_info_ptr& info)
{
	auto weak_this = weak_ptr(this->shared_from_this());

	connection_info conn_info;
	conn_info.connection = connection;

	// sentinel to be used to monitor if connection has been removed
	// instead of expensive lookup into the connections container.
	// this can also be used to
	conn_info.sentinel = std::make_shared<connection::id_t>(connection->id);

	connection->on_disconnect.emplace_back([weak_this, info](connection::id_t id, const error_code& ec) {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}

		shared_this->on_disconnect(id, ec, info);
	});

	auto sentinel = std::weak_ptr<void>(conn_info.sentinel);
	connection->on_msg.emplace_back([weak_this, info, sentinel](connection::id_t id, byte_buffer msg) {
		auto shared_this = weak_this.lock();
		if(!shared_this || sentinel.expired())
		{
			return;
		}

		shared_this->on_msg(id, msg, info);
	});

	on_connect(connection->id, std::move(conn_info), info);

	connection->start();
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::on_connect(connection::id_t id, connection_info&& conn_info,
												  const user_info_ptr& info)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		connections_.emplace(id, std::move(conn_info));
	}

	if(info->on_connect)
	{
		itc::invoke(info->thread_id, info->on_connect, id);
	}
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::on_disconnect(connection::id_t id, error_code ec,
													 const user_info_ptr& info)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		connections_.erase(id);
	}
	if(info->on_disconnect)
	{
		itc::invoke(info->thread_id, info->on_disconnect, id, ec);
	}
}

template <typename T, typename OArchive, typename IArchive>
void messenger<T, OArchive, IArchive>::on_msg(connection::id_t id, byte_buffer& msg,
											  const user_info_ptr& info)
{
	if(info->on_msg)
	{
		itc::invoke(info->thread_id, info->on_msg, id, serializer_t::from_buffer(std::move(msg)));
	}
}

template <typename T, typename OArchive, typename IArchive>
typename messenger<T, OArchive, IArchive>::ptr get_network()
{
	using messenger_type = messenger<T, OArchive, IArchive>;
	static std::mutex messenger_mutex;

	static typename messenger_type::ptr net = []() {
		auto& mutex = get_messengers_mutex();
		auto& deleters = get_deleters();
		std::lock_guard<std::mutex> lock(mutex);
		deleters.emplace_back([]() {
			std::lock_guard<std::mutex> lock(messenger_mutex);
			net->remove_all();
			net.reset();
		});
		return messenger_type::create();
	}();

	std::lock_guard<std::mutex> lock(messenger_mutex);
	return net;
}

} // namespace net
