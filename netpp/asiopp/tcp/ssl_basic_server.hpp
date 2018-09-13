#pragma once
#include "basic_server.hpp"
#include <asio/ssl.hpp>

#include <iostream>
namespace net
{
namespace tcp
{
template <typename protocol_type>
class ssl_basic_server : public basic_server<protocol_type>
{
public:
    using base_type = basic_server<protocol_type>;
    using protocol = typename base_type::protocol;
    using weak_ptr = typename base_type::weak_ptr;
    using protocol_endpoint = typename base_type::protocol_endpoint;
    using protocol_acceptor = typename base_type::protocol_acceptor;
    using protocol_socket = typename base_type::protocol_socket;

    ssl_basic_server(asio::io_context& io_context, const protocol_endpoint& listen_endpoint,
                     const std::string& cert_chain_file, const std::string& private_key_file,
                     const std::string& dh_file)
        : base_type(io_context, listen_endpoint)
        , context_(asio::ssl::context::sslv23)
    {
        context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                             asio::ssl::context::single_dh_use);
        context_.set_password_callback(std::bind(&ssl_basic_server::get_password, this));
        context_.use_certificate_chain_file(cert_chain_file);
        context_.use_private_key_file(private_key_file, asio::ssl::context::pem);
        context_.use_tmp_dh_file(dh_file);
    }

    void start()
    {
        using socket_type = asio::ssl::stream<protocol_socket>;
        auto socket = std::make_shared<socket_type>(this->io_context_, context_);

        auto weak_this = weak_ptr(this->shared_from_this());
        auto on_connection_established = [weak_this, socket]() mutable {
            // Start the asynchronous handshake operation.
            socket->async_handshake(asio::ssl::stream_base::server,
                                    [weak_this, socket](const error_code& ec) mutable {
                                        if(ec)
                                        {
                                            std::cout << "Handshake error: " << ec.message() << std::endl;

                                            // We need to close the socket used in the previous connection
                                            // attempt
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

        this->async_accept(socket, std::move(on_connection_established));
    }

protected:
    std::string get_password() const
    {
        return "test";
    }
    asio::ssl::context context_;
};
}
} // namespace net
