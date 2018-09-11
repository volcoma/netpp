#include "tcp_server.h"
#include "tcp_connection.hpp"

#include <iostream>

namespace net
{

tcp_server::tcp_server(asio::io_context& io_context, const tcp::endpoint& listen_endpoint)
	: connector(io_context)
	, acceptor_(io_context, listen_endpoint)
{
}

void tcp_server::start()
{
	using socket_type = tcp::socket;

	auto socket = std::make_shared<socket_type>(io_context_);
    auto close_socket = [](socket_type& socket) mutable
    {
        asio::error_code ignore;
        socket.shutdown(socket_base::shutdown_both, ignore);
        socket.close(ignore);
    };
	acceptor_.async_accept(*socket, [this, socket, close_socket](const asio::error_code& ec) mutable {
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
            auto session = std::make_shared<tcp_connection<socket_type>>(socket, io_context_);
            session->on_connect_ = [this](connection_ptr session)
            {
                channel_.join(session);
                on_connect(session->get_id());
            };

            session->on_disconnect_ = [this, socket, close_socket](connection_ptr session, const error_code& ec) mutable
            {
                close_socket(*socket);
                socket.reset();

                channel_.leave(session);
                on_disconnect(session->get_id(), ec);
            };

            session->on_msg_ = [this](connection_ptr session, const byte_buffer& msg)
            {
                on_msg(session->get_id(), msg);
            };

            session->start();
		}

		// Try again.
		start();
	});
}

} // namespace net
