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
	if(connector->on_connect)
	{
		return connector_id;
	}
	connector_info info;
	info.connector = connector;
	info.on_connect = std::move(on_connect);
	info.on_disconnect = std::move(on_disconnect);
	info.on_msg = std::move(on_msg);

	auto weak_this = weak_ptr(shared_from_this());
	connector->on_connect = [weak_this, connector_id](connection::id_t id) {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}

		shared_this->on_connect(connector_id, id);
	};

	connector->on_disconnect = [weak_this, connector_id](connection::id_t id, const error_code& ec) {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}

		shared_this->on_disconnect(connector_id, id, ec);
	};

	connector->on_msg = [weak_this, connector_id](connection::id_t id, const byte_buffer& msg) {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}

		shared_this->on_msg(connector_id, id, msg);
	};

	connectors_.emplace(connector->id, std::move(info));
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
	auto& connector_id = conn_it->second;

	auto it = connectors_.find(connector_id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;
	auto connector = info.connector;

	lock.unlock();

	connector->send_msg(id, std::move(msg));
}

void messenger::disconnect(connection::id_t id)
{
	std::unique_lock<std::mutex> lock(guard_);

	auto conn_it = connections_.find(id);
	if(conn_it == std::end(connections_))
	{
		return;
	}
	auto& connector_id = conn_it->second;

	auto it = connectors_.find(connector_id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;
	auto connector = info.connector;

	lock.unlock();

	connector->stop(id, {});
}

void messenger::remove_connector(connector::id_t id)
{
	std::unique_lock<std::mutex> lock(guard_);
	auto it = connectors_.find(id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;
	auto connector = info.connector;
	connectors_.erase(it);
	lock.unlock();

	connector->stop(id, {});
}

void messenger::on_connect(connector::id_t connector_id, connection::id_t id)
{
	std::unique_lock<std::mutex> lock(guard_);
	connections_[id] = connector_id;
	auto it = connectors_.find(connector_id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;

	lock.unlock();
	if(info.on_connect)
	{
		info.on_connect(id);
	}
}

void messenger::on_disconnect(connector::id_t connector_id, connection::id_t id, error_code ec)
{
	std::unique_lock<std::mutex> lock(guard_);

	auto it = connectors_.find(connector_id);
	if(it == std::end(connectors_))
	{
		return;
	}
	connections_.erase(id);
	auto& info = it->second;
	lock.unlock();

	if(info.on_disconnect)
	{
		info.on_disconnect(id, ec);
	}
}

void messenger::on_msg(connector::id_t connector_id, connection::id_t id, const byte_buffer& msg)
{
	std::unique_lock<std::mutex> lock(guard_);

	auto it = connectors_.find(connector_id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;
	lock.unlock();

	if(info.on_msg)
	{
		info.on_msg(id, msg);
	}
}

} // namespace net
