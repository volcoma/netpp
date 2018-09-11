#include "channel.h"
#include <atomic>

namespace net
{

connection::connection()
{
    static std::atomic<id> ids{0};
	id_ = ++ids;
}

const connection::id& connection::get_id() const
{
	return id_;
}

void channel::join(const channel::connection_ptr& conn)
{
    std::lock_guard<std::mutex> lock(guard_);
	connections_.emplace(conn->get_id(), conn);
}

void channel::leave(const channel::connection_ptr& conn)
{
    std::lock_guard<std::mutex> lock(guard_);
	connections_.erase(conn->get_id());
}

void channel::stop(const error_code& ec)
{
    auto connections = [this]()
    {
        std::lock_guard<std::mutex> lock(guard_);
        return std::move(connections_);
    }();

	for(auto& kvp : connections)
	{
		kvp.second->stop(ec);
	}

}

void channel::stop(connection::id id, const error_code& ec)
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

void channel::send_msg(connection::id id, byte_buffer&& msg)
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

} // namespace net
