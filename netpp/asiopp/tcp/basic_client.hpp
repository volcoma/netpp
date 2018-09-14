#pragma once
#include "../connector.h"
#include "connection.hpp"


#include <asio/basic_socket_acceptor.hpp>
#include <asio/basic_stream_socket.hpp>

namespace net
{
namespace tcp
{

template <typename protocol_type>
class basic_client : public asio_connector, public std::enable_shared_from_this<basic_client<protocol_type>>
{
public:
    using protocol = protocol_type;
    using weak_ptr = std::weak_ptr<basic_client<protocol_type>>;
    using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
    using protocol_endpoint = typename protocol_type::endpoint;
    using protocol_socket = typename protocol_type::socket;

    basic_client(asio::io_context& io_context, const protocol_endpoint& endpoint)
        : asio_connector(io_context)
        , endpoint_(endpoint)
    {
    }

    void start() override
    {
        using socket_type = protocol_socket;
        auto socket = std::make_shared<socket_type>(io_context_);

        auto weak_this = weak_ptr(this->shared_from_this());
        auto on_connection_established = [ weak_this, socket ]()
        {
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }
            shared_this->on_handshake_complete(socket);
        };

        async_connect(socket, std::move(on_connection_established));
    }

    template <typename socket_type, typename F>
    void async_connect(std::shared_ptr<socket_type> socket, F f)
    {
        sout() << "trying to connect to " << endpoint_ << "..."
               << "\n";

        auto weak_this = weak_ptr(this->shared_from_this());

        socket->lowest_layer().async_connect(
            endpoint_,
            [ weak_this, socket, on_connection_established = std::move(f) ](const error_code& ec) mutable {
                // The async_connect() function automatically opens the socket at the start
                // of the asynchronous operation.

                // Check if the connect operation failed before the deadline expired.
                if(ec)
                {
                    // std::cout << "Connect error: " << ec.message() << "\n";
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
                    on_connection_established();
                }
            });
    }

    template <typename socket_type>
    void on_handshake_complete(std::shared_ptr<socket_type> socket)
    {
        sout() << "handshake " << socket->lowest_layer().local_endpoint() << " -> "
               << socket->lowest_layer().remote_endpoint() << " completed"
               << "\n";

        auto session = std::make_shared<async_connection<socket_type>>(socket, io_context_);
        auto weak_this = weak_ptr(this->shared_from_this());

        session->on_connect = [weak_this](connection::id_t id) mutable {
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }

            if(shared_this->on_connect)
            {
                shared_this->on_connect(id);
            }
        };

        session->on_disconnect = [weak_this](connection::id_t id, const error_code& ec) mutable {
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }
            shared_this->remove(id);
            if(shared_this->on_disconnect)
            {
                shared_this->on_disconnect(id, ec);
            }
            // Try again
            shared_this->start();
        };

        session->on_msg = [weak_this](connection::id_t id, const byte_buffer& msg) mutable {
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }
            if(shared_this->on_msg)
            {
                shared_this->on_msg(id, msg);
            }
        };

        add(session);

        session->start();
    }

protected:
    protocol_endpoint endpoint_;
};
}
} // namespace net
