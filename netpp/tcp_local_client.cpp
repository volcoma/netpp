#include "tcp_local_client.h"
#include "tcp_connection.hpp"

#include <iostream>

#ifdef ASIO_HAS_LOCAL_SOCKETS

namespace net
{

//local_client::local_client(asio::io_context& io_context, stream_protocol::endpoint endpoint)
//	: connector(io_context)
//	, endpoint_(std::move(endpoint))
//{
//}

//void local_client::start()
//{
//	const auto& entry = endpoint_;
//	std::cout << "Trying " << entry << "..." << std::endl;

//	using socket_type = basic_stream_socket<stream_protocol>;
//	auto socket = std::make_shared<socket_type>(io_context_);
//	auto session = std::make_shared<tcp_connection<socket_type>>(socket, io_context_, *this);

//	// Start the asynchronous connect operation.
//	socket->async_connect(entry, [this, session](const error_code& ec) {
//		// The async_connect() function automatically opens the socket at the start
//		// of the asynchronous operation. If the socket is closed at this time then
//		// the timeout handler must have run first.
//		if(session->stopped())
//		{
//			std::cout << "Connect timed out" << std::endl;

//			// Try again.
//			start();
//		}
//		// Check if the connect operation failed before the deadline expired.
//		else if(ec)
//		{
//			// std::cout << "Connect error: " << ec.message() << std::endl;

//			// We need to close the socket used in the previous connection attempt
//			// before starting a new one.
//			session->stop(ec);

//			// Try again.
//			start();
//		}

//		// Otherwise we have successfully established a connection.
//		else
//		{
//			session->start();
//		}
//	});
//}

} // namespace net
#endif
