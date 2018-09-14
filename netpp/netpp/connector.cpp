#include "connector.h"
#include <atomic>
namespace net
{
static connector::id_t get_next_id()
{
	static std::atomic<connector::id_t> ids{0};
	return ++ids;
}

connector::connector()
	: id(get_next_id())
{
}

void connector::stop(const std::error_code& ec)
{
	auto connections = [&]() {
		std::lock_guard<std::mutex> lock(guard_);
		return std::move(connections_);
	}();

	for(auto& kvp : connections)
	{
		kvp.second->stop(ec);
	}
}

void connector::stop(connection::id_t id, const std::error_code& ec)
{
	std::unique_lock<std::mutex> lock(guard_);
	auto it = connections_.find(id);
	if(it != std::end(connections_))
	{
		auto connection = it->second;
		lock.unlock();
		connection->stop(ec);
	}
}

void connector::send_msg(connection::id_t id, byte_buffer&& msg)
{
	std::unique_lock<std::mutex> lock(guard_);
	auto it = connections_.find(id);
	if(it != std::end(connections_))
	{
		auto connection = it->second;
		lock.unlock();
		connection->send_msg(std::move(msg));
	}
}

void connector::add(const connection_ptr& session)
{
	std::lock_guard<std::mutex> lock(guard_);
	connections_.emplace(session->id, session);
}

void connector::remove(connection::id_t id)
{
	std::lock_guard<std::mutex> lock(guard_);
	connections_.erase(id);
}

} // namespace net
