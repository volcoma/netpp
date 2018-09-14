#pragma once
#include "basic_client.hpp"
#include <asio/ssl.hpp>

namespace net
{
namespace tcp
{
namespace detail
{
inline bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx)
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
    sout() << "Verifying " << subject_name << "\n";

    return preverified;
}
}

template <typename protocol_type>
class basic_ssl_client : public basic_client<protocol_type>
{
public:
    using base_type = basic_client<protocol_type>;
    using protocol = typename base_type::protocol;
    using weak_ptr = typename base_type::weak_ptr;
    using protocol_endpoint = typename base_type::protocol_endpoint;
    using protocol_acceptor = typename base_type::protocol_acceptor;
    using protocol_socket = typename base_type::protocol_socket;

    basic_ssl_client(asio::io_context& io_context, const protocol_endpoint& listen_endpoint,
                     const std::string& cert_file)
        : base_type(io_context, listen_endpoint)
        , context_(asio::ssl::context::sslv23)
    {
        context_.load_verify_file(cert_file);
    }

    void start()
    {
        using socket_type = asio::ssl::stream<protocol_socket>;
        auto socket = std::make_shared<socket_type>(this->io_context_, context_);
        socket->set_verify_mode(asio::ssl::verify_peer);
        socket->set_verify_callback(detail::verify_certificate);

        auto weak_this = weak_ptr(this->shared_from_this());
        auto on_connection_established = [ weak_this, socket ]() mutable
        {
            // Start the asynchronous handshake operation.
            socket->async_handshake(
                asio::ssl::stream_base::client,
                [ weak_this, socket ](const error_code& ec) mutable {
                    if(ec)
                    {
                        sout() << "Handshake error: " << ec.message() << "\n";

                        // We need to close the socket used in the previous connection
                        // attempt before starting a new one.
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

        this->async_connect(socket, std::move(on_connection_established));
    }

protected:
    asio::ssl::context context_;
};
}
} // namespace net
