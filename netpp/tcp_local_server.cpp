#include "tcp_local_server.h"
#include "tcp_connection.hpp"

#include <iostream>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{

local_server::local_server(asio::io_context& io_context, const stream_protocol::endpoint& listen_endpoint)
	: connector(io_context)
	, acceptor_(io_context, listen_endpoint)
{
    acceptor_.set_option(socket_base::reuse_address(true));
}

void local_server::start()
{
	using socket_type = basic_stream_socket<stream_protocol>;
	auto socket = std::make_shared<socket_type>(io_context_);
    auto close_socket = [](socket_type& socket) mutable
    {
        asio::error_code ignore;
        socket.shutdown(socket_base::shutdown_both, ignore);
        socket.close(ignore);
    };

    auto weak_this = connector_weak_ptr(this->shared_from_this());
    acceptor_.async_accept(*socket, [weak_this, socket, close_socket](const asio::error_code& ec) mutable {
		if(ec)
		{
			std::cout << "Accept error: " << ec.message() << std::endl;

			// We need to close the socket used in the previous connection attempt
			// before starting a new one.
            close_socket(*socket);
            socket.reset();
		}
		// Otherwise we have successfully established a connection.
		else
		{
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }
            auto session = std::make_shared<tcp_connection<socket_type>>(socket, shared_this->io_context_);
            session->on_connect_ = [weak_this](connection_ptr session) mutable
            {
                auto shared_this = weak_this.lock();
                if(!shared_this)
                {
                    return;
                }
                shared_this->channel_.join(session);
                shared_this->on_connect(session->get_id());
            };

            session->on_disconnect_ = [weak_this, socket, close_socket](connection_ptr session, const error_code& ec) mutable
            {
                close_socket(*socket);
                socket.reset();

                auto shared_this = weak_this.lock();
                if(!shared_this)
                {
                    return;
                }
                shared_this->channel_.leave(session);
                shared_this->on_disconnect(session->get_id(), ec);
            };

            session->on_msg_ = [weak_this](connection_ptr session, const byte_buffer& msg) mutable
            {
                auto shared_this = weak_this.lock();
                if(!shared_this)
                {
                    return;
                }
                shared_this->on_msg(session->get_id(), msg);
            };

            session->start();
		}
        auto shared_this = weak_this.lock();
        if(!shared_this)
        {
            return;
        }
		// Try again.
		shared_this->start();
	});
}

} // namespace net
#endif
