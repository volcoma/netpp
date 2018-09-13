#include "tcp_ssl_local_server.h"
#include "tcp_connection.hpp"

#include <iostream>
#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{

tcp_ssl_local_server::tcp_ssl_local_server(asio::io_context& io_context, const protocol_endpoint& listen_endpoint,
							   const std::string& cert_chain_file, const std::string& private_key_file,
							   const std::string& dh_file)
	: tcp_local_server(io_context, listen_endpoint)
	, context_(asio::ssl::context::sslv23)
{
	context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
						 asio::ssl::context::single_dh_use);
	context_.set_password_callback(std::bind(&tcp_ssl_local_server::get_password, this));
	context_.use_certificate_chain_file(cert_chain_file);
	context_.use_private_key_file(private_key_file, asio::ssl::context::pem);
	context_.use_tmp_dh_file(dh_file);
}

void tcp_ssl_local_server::start()
{
	using socket_type = asio::ssl::stream<protocol::socket>;
	auto socket = std::make_shared<socket_type>(io_context, context_);

	auto weak_this = weak_ptr(shared_from_this());
	auto on_connection_established = [weak_this, socket]() mutable {
		// Start the asynchronous handshake operation.
		socket->async_handshake(asio::ssl::stream_base::server,
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

	async_accept(socket, std::move(on_connection_established));
}

std::string tcp_ssl_local_server::get_password() const
{
	return "test";
}

} // namespace net
#endif
