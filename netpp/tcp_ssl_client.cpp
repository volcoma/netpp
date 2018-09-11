#include "tcp_ssl_client.h"
#include "tcp_connection.hpp"
#include <iostream>

namespace net
{

tcp_ssl_client::tcp_ssl_client(asio::io_context& io_context, tcp::endpoint endpoint,
							   const std::string& cert_file)
	: connector(io_context)
	, endpoint_(std::move(endpoint))
	, context_(asio::ssl::context::sslv23)
{
	context_.load_verify_file(cert_file);
}

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

void tcp_ssl_client::start()
{
	std::cout << "Trying " << endpoint_ << "..." << std::endl;

	using socket_type = asio::ssl::stream<tcp::socket>;
	auto socket = std::make_shared<socket_type>(io_context_, context_);
	socket->set_verify_mode(asio::ssl::verify_peer);
	socket->set_verify_callback(verify_certificate);

	auto close_socket = [](socket_type& socket) mutable {
		asio::error_code ignore;
		socket.shutdown(ignore);
		socket.lowest_layer().shutdown(socket_base::shutdown_both, ignore);
		socket.lowest_layer().close(ignore);
	};
    auto weak_this = connector_weak_ptr(this->shared_from_this());
	// Start the asynchronous connect operation.
	socket->lowest_layer().async_connect(endpoint_, [weak_this, socket,
													 close_socket](const error_code& ec) mutable {
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
			// Start the asynchronous handshake operation.
            socket->async_handshake(
				asio::ssl::stream_base::client, [weak_this, socket, close_socket](const error_code& ec) mutable {
					if(ec)
					{
						std::cout << "Handshake error: " << ec.message() << std::endl;

						// We need to close the socket used in the previous connection attempt
						// before starting a new one.
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
	});
}

} // namespace net
