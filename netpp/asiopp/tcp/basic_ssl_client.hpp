#pragma once
#include "basic_client.hpp"
#include "basic_ssl_entity.hpp"
#include "compatibility.hpp"
namespace net
{
namespace tcp
{

template <typename protocol_type>
class basic_ssl_client : public basic_client<protocol_type>, public basic_ssl_entity
{
public:
    using base_type = basic_client<protocol_type>;
    using protocol = typename base_type::protocol;
    using weak_ptr = typename base_type::weak_ptr;
    using protocol_endpoint = typename base_type::protocol_endpoint;
    using protocol_acceptor = typename base_type::protocol_acceptor;
    using protocol_socket = typename base_type::protocol_socket;

    //-----------------------------------------------------------------------------
    /// Constructor of ssl client accepting a connect endpoint and certificates.
    //-----------------------------------------------------------------------------
    basic_ssl_client(asio::io_service& io_context, const protocol_endpoint& endpoint,
                     const ssl_config& config, std::chrono::seconds heartbeat = std::chrono::seconds{0}, bool auto_reconnect = true);

    //-----------------------------------------------------------------------------
    /// Starts the client attempting to connect to an endpoint.
    /// After a successful connect, a ssl handshake is performed to validate
    /// certificates and keys
    //-----------------------------------------------------------------------------
    void start() override;
};

template <typename protocol_type>
inline basic_ssl_client<protocol_type>::basic_ssl_client(asio::io_service& io_context,
                                                         const protocol_endpoint& endpoint,
                                                         const ssl_config& config,
                                                         std::chrono::seconds heartbeat,
                                                         bool auto_reconnect)
    : base_type(io_context, endpoint, heartbeat, auto_reconnect)
    , basic_ssl_entity(config)
{
}

template <typename protocol_type>
inline void basic_ssl_client<protocol_type>::start()
{
    auto socket = compatibility::make_socket<protocol_socket>(this->io_context_);
    auto weak_this = weak_ptr(this->shared_from_this());

    auto on_connection_established = [ weak_this, this, socket = socket.get() ]() mutable
    {
        auto shared_this = weak_this.lock();
        if(!shared_this)
        {
            return;
        }
        auto ssl_socket = compatibility::make_ssl_socket(std::move(*socket), context_);

        auto hanshake_type = [this]() {
			switch(config_.handshake_type)
			{
				case ssl_handshake::server:
					return asio::ssl::stream_base::server;
				case ssl_handshake::client:
					return asio::ssl::stream_base::client;

				// this is a client unless specified otherwise
				default:
					return asio::ssl::stream_base::client;
			}
		}();
        // Start the asynchronous handshake operation.
        ssl_socket->async_handshake(hanshake_type,
                                    [weak_this, ssl_socket](const error_code& ec) mutable {
                                        if(ec)
                                        {
                                            log() << "Handshake error: " << ec.message();

                                            auto shared_this = weak_this.lock();
                                            if(!shared_this)
                                            {
                                                return;
                                            }

                                            // We need to close the socket used in the previous connection
                                            // attempt before starting a new one.
                                            ssl_socket.reset();

                                            // Try again.
                                            shared_this->restart();
                                        }

                                        // Otherwise we have successfully established a connection.
                                        else
                                        {
                                            auto shared_this = weak_this.lock();
                                            if(!shared_this)
                                            {
                                                return;
                                            }

                                            shared_this->on_handshake_complete(ssl_socket);
                                        }
                                    });
    };

    this->async_connect(socket, std::move(on_connection_established));
}
}
} // namespace net
