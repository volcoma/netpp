#include "messenger.h"

namespace net
{

connector::id messenger::add_connector(const connector_ptr& connector, on_connect_t on_connect,
									   on_disconnect_t on_disconnect, on_msg_t on_msg)
{
	if(!connector)
	{
		return 0;
	}
	auto connector_id = connector->get_id();
	if(connector->on_connect_)
	{
		return connector_id;
	}
	connector_info info;
	info.connector = connector;
	info.on_connect = std::move(on_connect);
	info.on_disconnect = std::move(on_disconnect);
	info.on_msg = std::move(on_msg);

	connector->on_connect_ = [this, connector_id](connection::id id) {
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
	};

	connector->on_disconnect_ = [this, connector_id](connection::id id, asio::error_code ec) {
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
	};

	connector->on_msg_ = [this, connector_id](connection::id id, const byte_buffer& msg) {
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
	};

	connectors_.emplace(connector->get_id(), std::move(info));
	connector->start();
	return connector_id;
}

void messenger::send_msg(connection::id id, byte_buffer&& msg)
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

void messenger::disconnect(connection::id id)
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

    connector->stop(id);
}

void messenger::remove_connector(connector::id id)
{
    std::unique_lock<std::mutex> lock(guard_);
    auto it = connectors_.find(id);
	if(it == std::end(connectors_))
	{
		return;
	}
	auto& info = it->second;
    auto connector = info.connector;
    //lock.unlock();

    connector->stop(id);
    connectors_.erase(it);
}

} // namespace net
