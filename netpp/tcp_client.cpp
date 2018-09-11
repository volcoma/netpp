#include "tcp_client.h"
#include "tcp_connection.hpp"
#include <iostream>

namespace net
{

tcp_client::tcp_client(asio::io_context& io_context, tcp::endpoint endpoint)
	: connector(io_context)
	, endpoint_(std::move(endpoint))
{
}

void tcp_client::start()
{
	std::cout << "Trying " << endpoint_ << "..." << std::endl;

	using socket_type = tcp::socket;
	auto socket = std::make_shared<socket_type>(io_context_);

	auto close_socket = [](socket_type& socket) mutable {
		asio::error_code ignore;
		socket.shutdown(socket_base::shutdown_both, ignore);
		socket.close(ignore);
	};

    auto weak_this = connector_weak_ptr(this->shared_from_this());
	// Start the asynchronous connect operation.
	socket->async_connect(endpoint_, [weak_this, socket, close_socket](const error_code& ec) mutable {
		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation.

		// Check if the connect operation failed before the deadline expired.
		if(ec)
		{
			// std::cout << "Connect error: " << ec.message() << std::endl;
			close_socket(*socket);
			socket.reset();

            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }
			// Try again.
			shared_this->start();
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
	});
}

} // namespace net
