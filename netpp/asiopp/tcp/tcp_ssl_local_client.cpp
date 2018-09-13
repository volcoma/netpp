#include "tcp_ssl_local_client.h"

#include <iostream>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{
namespace
{
bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.

	// In this example we will simply print the certificate's subject name.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cout << "Verifying " << subject_name << std::endl;

	return preverified;
}
}

tcp_ssl_local_client::tcp_ssl_local_client(asio::io_context& io_context, const protocol_endpoint& endpoint,
							   const std::string& cert_file)
	: tcp_local_client(io_context, endpoint)
	, context_(asio::ssl::context::sslv23)
{
	context_.load_verify_file(cert_file);
}

void tcp_ssl_local_client::start()
{
	using socket_type = asio::ssl::stream<protocol::socket>;
	auto socket = std::make_shared<socket_type>(io_context, context_);
	socket->set_verify_mode(asio::ssl::verify_peer);
	socket->set_verify_callback(verify_certificate);

	auto weak_this = weak_ptr(shared_from_this());
	auto on_connection_established = [weak_this, socket]() mutable {
		// Start the asynchronous handshake operation.
		socket->async_handshake(asio::ssl::stream_base::client,
								[weak_this, socket](const error_code& ec) mutable {
									if(ec)
									{
										std::cout << "Handshake error: " << ec.message() << std::endl;

										// We need to close the socket used in the previous connection attempt
										// before starting a new one.
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
										shared_this->on_handshake_complete(socket);
									}
								});
	};

	async_connect(socket, std::move(on_connection_established));
}

} // namespace net
#endif
