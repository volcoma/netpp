#include "tcp_local_client.h"
#include "tcp_connection.hpp"

#include <iostream>

#ifdef ASIO_HAS_LOCAL_SOCKETS

namespace net
{

local_client::local_client(asio::io_context& io_context, stream_protocol::endpoint endpoint)
	: connector(io_context)
	, endpoint_(std::move(endpoint))
{
}

void local_client::start()
{
	std::cout << "Trying " << endpoint_ << "..." << std::endl;

	using socket_type = basic_stream_socket<stream_protocol>;
	auto socket = std::make_shared<socket_type>(io_context_);
    auto close_socket = [](socket_type& socket) mutable {
		asio::error_code ignore;
		socket.shutdown(socket_base::shutdown_both, ignore);
		socket.close(ignore);
	};
	// Start the asynchronous connect operation.
	socket->async_connect(endpoint_, [this, socket, close_socket](const error_code& ec) mutable {
		// The async_connect() function automatically opens the socket at the start
		// of the asynchronous operation.

		// Check if the connect operation failed before the deadline expired.
		if(ec)
		{
			// std::cout << "Connect error: " << ec.message() << std::endl;
			close_socket(*socket);
			socket.reset();
			// Try again.
			start();
		}

		// Otherwise we have successfully established a connection.
		else
		{
			auto session = std::make_shared<tcp_connection<socket_type>>(socket, io_context_);
			session->on_connect_ = [this](connection_ptr session) {
				channel_.join(session);
				on_connect(session->get_id());
			};

			session->on_disconnect_ = [this, socket, close_socket](connection_ptr session,
																   const error_code& ec) mutable {
				close_socket(*socket);
				socket.reset();

				channel_.leave(session);
				on_disconnect(session->get_id(), ec);
			};

			session->on_msg_ = [this](connection_ptr session, const byte_buffer& msg) {
				on_msg(session->get_id(), msg);
			};

			session->start();
		}
	});
}

} // namespace net
#endif
