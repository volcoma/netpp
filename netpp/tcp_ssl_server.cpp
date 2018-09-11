#include "tcp_ssl_server.h"
#include "tcp_connection.hpp"

#include <iostream>

namespace net
{

tcp_ssl_server::tcp_ssl_server(asio::io_context& io_context, const tcp::endpoint& listen_endpoint,
							   const std::string& cert_chain_file, const std::string& private_key_file,
							   const std::string& dh_file)
	: connector(io_context)
	, acceptor_(io_context, listen_endpoint)
	, context_(asio::ssl::context::sslv23)
{
	context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
						 asio::ssl::context::single_dh_use);
	context_.set_password_callback(std::bind(&tcp_ssl_server::get_password, this));
	context_.use_certificate_chain_file(cert_chain_file);
	context_.use_private_key_file(private_key_file, asio::ssl::context::pem);
	context_.use_tmp_dh_file(dh_file);
}

void tcp_ssl_server::start()
{
	using socket_type = asio::ssl::stream<tcp::socket>;
	auto socket = std::make_shared<socket_type>(io_context_, context_);
	auto close_socket = [](socket_type& socket) mutable {
		asio::error_code ignore;
		socket.shutdown(ignore);
		socket.lowest_layer().shutdown(socket_base::shutdown_both, ignore);
		socket.lowest_layer().close(ignore);
	};
	acceptor_.async_accept(socket->lowest_layer(), [this, socket,
													close_socket](const asio::error_code& ec) mutable {
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
			// Start the asynchronous handshake operation.
			socket->async_handshake(
				asio::ssl::stream_base::client, [this, socket, close_socket](const error_code& ec) mutable {
					if(ec)
					{
						std::cout << "Handshake error: " << ec.message() << std::endl;

						// We need to close the socket used in the previous connection attempt
						// before starting a new one.
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

		// Try again.
		start();
	});
}

std::string tcp_ssl_server::get_password() const
{
	return "test";
}

} // namespace net
