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
    acceptor_.set_option(socket_base::reuse_address(true));

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
    auto weak_this = connector_weak_ptr(this->shared_from_this());
	acceptor_.async_accept(socket->lowest_layer(), [weak_this, socket,
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
				asio::ssl::stream_base::server, [weak_this, socket, close_socket](const error_code& ec) mutable {
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

        auto shared_this = weak_this.lock();
        if(!shared_this)
        {
            return;
        }
		// Try again.
		shared_this->start();
	});
}

std::string tcp_ssl_server::get_password() const
{
	return "test";
}

} // namespace net
