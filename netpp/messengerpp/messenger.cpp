#include "messenger.h"

namespace net
{

messenger::ptr messenger::create()
{
	struct make_shared_enabler : messenger
	{
	};
	return std::make_shared<make_shared_enabler>();
}

connector::id_t messenger::add_connector(const connector_ptr& connector, on_connect_t on_connect,
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

	auto weak_this = weak_ptr(shared_from_this());

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

void messenger::send_msg(connection::id_t id, byte_buffer&& msg)
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

	connection->send_msg(std::move(msg), 0);
}

void messenger::disconnect(connection::id_t id)
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

void messenger::remove_connector(connector::id_t id)
{
	std::lock_guard<std::mutex> lock(guard_);
	connectors_.erase(id);
}

bool messenger::empty() const
{
	std::lock_guard<std::mutex> lock(guard_);
	return connectors_.empty() && connections_.empty();
}

void messenger::stop()
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

void messenger::on_new_connection(connection_ptr& connection, const user_info_ptr& info)
{
	auto weak_this = weak_ptr(shared_from_this());

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
	connection->on_msg.emplace_back([weak_this, info, sentinel](connection::id_t id, const byte_buffer& msg) {
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

void messenger::on_connect(connection::id_t id, connection_info&& conn_info, const user_info_ptr& info)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		connections_.emplace(id, std::move(conn_info));
	}

	if(info->on_connect)
	{
		info->on_connect(id);
	}
}

void messenger::on_disconnect(connection::id_t id, error_code ec, const user_info_ptr& info)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		connections_.erase(id);
	}
	if(info->on_disconnect)
	{
		info->on_disconnect(id, ec);
	}
}

void messenger::on_msg(connection::id_t id, const byte_buffer& msg, const user_info_ptr& info)
{
	if(info->on_msg)
	{
		info->on_msg(id, msg);
	}
}
std::vector<std::function<void()>>& get_deleters()
{
	static std::vector<std::function<void()>> deleters;
	return deleters;
}

std::mutex& get_messengers_mutex()
{
	static std::mutex s_mutex;
	return s_mutex;
}
messenger::ptr get_network()
{
	static std::mutex messenger_mutex;

	static net::messenger::ptr net = []() {
		auto& mutex = get_messengers_mutex();
		auto& deleters = get_deleters();
		std::lock_guard<std::mutex> lock(mutex);
		deleters.emplace_back([]() {
			std::lock_guard<std::mutex> lock(messenger_mutex);
			net->stop();
			net.reset();
		});
		return net::messenger::create();
	}();

	std::lock_guard<std::mutex> lock(messenger_mutex);
	return net;
}

void deinit_messengers()
{
	auto& mutex = get_messengers_mutex();
	std::unique_lock<std::mutex> lock(mutex);
	auto deleters = std::move(get_deleters());
	lock.unlock();
	for(auto& deleter : deleters)
	{
		deleter();
	}
}

} // namespace net
