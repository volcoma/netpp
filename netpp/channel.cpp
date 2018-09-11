#include "channel.h"

namespace net
{

connection::connection()
{
	static id ids = 0;
	id_ = ++ids;
}

const connection::id& connection::get_id() const
{
	return id_;
}

void channel::join(const channel::connection_ptr& conn)
{
	connections_.emplace(conn->get_id(), conn);
}

void channel::leave(const channel::connection_ptr& conn)
{
	connections_.erase(conn->get_id());
}

void channel::stop(const error_code& ec)
{
	for(auto& kvp : connections_)
	{
		kvp.second->stop(ec);
	}

	connections_.clear();
}

void channel::send_msg(connection::id id, byte_buffer&& msg)
{
	auto it = connections_.find(id);
	if(it != std::end(connections_))
	{
		auto& connection = it->second;
		connection->send_msg(std::move(msg));
	}
}

} // namespace net
