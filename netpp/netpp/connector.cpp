#include "connector.h"
#include <atomic>
namespace net
{
static connector::id_t get_next_id()
{
	static std::atomic<connector::id_t> ids{0};
	return ++ids;
}

void connections::join(const connection_ptr& session)
{
	std::lock_guard<std::mutex> lock(guard_);
	connections_.emplace(session->id, session);
}

void connections::leave(const connection_ptr& session)
{
	std::lock_guard<std::mutex> lock(guard_);
	connections_.erase(session->id);
}

void connections::stop(const error_code& ec)
{
	auto connections = [this]() {
		std::lock_guard<std::mutex> lock(guard_);
		return std::move(connections_);
	}();

	for(auto& kvp : connections)
	{
		kvp.second->stop(ec);
	}
}

void connections::stop(connection::id_t id, const error_code& ec)
{
	std::unique_lock<std::mutex> lock(guard_);
	auto it = connections_.find(id);
	if(it != std::end(connections_))
	{
		auto connection = it->second;
		lock.unlock();
		connection->stop(ec);
		lock.lock();
	}
}

void connections::send_msg(connection::id_t id, byte_buffer&& msg)
{
	std::unique_lock<std::mutex> lock(guard_);
	auto it = connections_.find(id);
	if(it != std::end(connections_))
	{
		auto connection = it->second;
		lock.unlock();
		connection->send_msg(std::move(msg));
		lock.lock();
	}
}

connector::connector()
	: id(get_next_id())
{
}

void connector::stop(const std::error_code& ec)
{
	channel.stop(ec);
}

void connector::stop(connection::id_t id, const std::error_code& ec)
{
	channel.stop(id, ec);
}

void connector::send_msg(connection::id_t id, byte_buffer&& msg)
{
	channel.send_msg(id, std::move(msg));
}

} // namespace net
