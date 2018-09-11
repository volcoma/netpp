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
	start();
}

void connector::on_connect(connection::id id)
{
	if(on_connect_)
	{
		on_connect_(id);
    }
}

void connector::setup_connection(connection& session)
{
    session.on_connect_ = [this](connection_ptr conn)
    {
        channel_.join(conn);
        on_connect(conn->get_id());
    };

    session.on_disconnect_ = [this](connection_ptr conn, const error_code& ec)
    {
        channel_.leave(conn);
        on_disconnect(conn->get_id(), ec);
    };

    session.on_msg_ = [this](connection_ptr conn, const byte_buffer& msg)
    {
        on_msg(conn->get_id(), msg);
    };
}

const connector::id& connector::get_id() const
{
	return id_;
}

} // namespace net
