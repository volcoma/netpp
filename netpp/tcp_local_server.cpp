#include "tcp_local_server.h"
#include "tcp_connection.hpp"

#include <iostream>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{

//local_server::local_server(asio::io_context& io_context, const stream_protocol::endpoint& listen_endpoint)
//	: connector(io_context)
//	, acceptor_(io_context, listen_endpoint)
//{
//}

//void local_server::start()
//{
//	using socket_type = basic_stream_socket<stream_protocol>;
//	auto socket = std::make_shared<socket_type>(io_context_);
//	auto session = std::make_shared<tcp_connection<socket_type>>(socket, io_context_, *this);

//	acceptor_.async_accept(*socket, [this, session](const asio::error_code& ec) mutable {
//		if(session->stopped())
//		{
//			std::cout << "Accept timed out" << std::endl;
//		}
//		// Check if the connect operation failed before the deadline expired.
//		else if(ec)
//		{
//			std::cout << "Accept error: " << ec.message() << std::endl;

//			// We need to close the socket used in the previous connection attempt
//			// before starting a new one.
//			session->stop(ec);
//		}
//		// Otherwise we have successfully established a connection.
//		else
//		{
//			session->start();
//		}

//		// Try again.
//		start();
//	});
//}

} // namespace net
#endif
