#include "connector.h"

namespace net
{

connector::connector(io_context& io_context)
	: io_context_(io_context)
{
    static std::atomic<id> ids{0};
	id_ = ++ids;
}

void connector::stop()
{
    channel_.stop(error::make_error_code(error::connection_aborted));
}

void connector::stop(connection::id id)
{
    channel_.stop(id, error::make_error_code(error::connection_aborted));
}

void connector::send_msg(connection::id id, byte_buffer&& msg)
{
	channel_.send_msg(id, std::move(msg));
}

void connector::on_msg(connection::id id, const byte_buffer& msg)
{
	if(on_msg_)
	{
		on_msg_(id, msg);
	}
}

void connector::on_disconnect(connection::id id, error_code ec)
{
	if(on_disconnect_)
	{
		on_disconnect_(id, ec);
	}
}

void connector::on_connect(connection::id id)
{
	if(on_connect_)
	{
		on_connect_(id);
    }
}

const connector::id& connector::get_id() const
{
	return id_;
}

} // namespace net
