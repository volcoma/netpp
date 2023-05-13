#include "basic_client.h"
#include "connection.h"
#include <asio/ip/multicast.hpp>

namespace net
{
namespace udp
{

basic_client::basic_client(asio::io_service& io_context, udp::endpoint endpoint,
                           std::chrono::seconds heartbeat, bool auto_reconnect)
    : endpoint_(std::move(endpoint))
    , io_context_(io_context)
    , reconnect_timer_(io_context)
    , heartbeat_(heartbeat)
{
    reconnect_timer_.expires_at(asio::steady_timer::time_point::max());
}

void basic_client::start()
{
    auto socket = std::make_shared<udp::socket>(io_context_);

    udp::endpoint socket_endpoint;
    asio::error_code ec;

    if(endpoint_.address().is_multicast())
    {
        if(endpoint_.address().is_v4())
        {
            socket_endpoint = udp::endpoint(asio::ip::address_v4::any(), endpoint_.port());
        }
        else if(endpoint_.address().is_v6())
        {
            socket_endpoint = udp::endpoint(asio::ip::address_v6::any(), endpoint_.port());
        }

        asio::error_code ec;
        if(socket->open(socket_endpoint.protocol(), ec))
        {
            log() << "[Error] datagram_socket::open : " << ec.message();
        }
        if(socket->set_option(asio::socket_base::reuse_address(true), ec))
        {
            log() << "[Error] datagram_socket::reuse_address : " << ec.message();
        }

        if(socket->bind(socket_endpoint, ec))
        {
            log() << "[Error] datagram_socket::bind : " << ec.message();
        }

        if(socket->set_option(asio::ip::multicast::join_group(endpoint_.address()), ec))
        {
            log() << "[Error] datagram_socket::multicast::join_group : " << ec.message();
        }

        //        if(socket->set_option(asio::ip::multicast::enable_loopback(false), ec))
        //        {
        //            log() << "[Error] datagram_socket::multicast::enable_loopback : " << ec.message();
        //        }

        if(socket->set_option(asio::ip::multicast::hops(5), ec))
        {
            log() << "[Error] datagram_socket::multicast::hops : " << ec.message();
        }
    }
    else
    {
        if(endpoint_.address().is_v4())
        {
            socket_endpoint = udp::endpoint(asio::ip::address_v4::any(), 0);
        }
        else if(endpoint_.address().is_v6())
        {
            socket_endpoint = udp::endpoint(asio::ip::address_v6::any(), 0);
        }

        if(socket->open(socket_endpoint.protocol(), ec))
        {
            log() << "[Error] datagram_socket::open : " << ec.message();
        }
        if(socket->set_option(asio::socket_base::reuse_address(true), ec))
        {
            log() << "[Error] datagram_socket::reuse_address : " << ec.message();
        }

        if(socket->bind(socket_endpoint, ec))
        {
            log() << "[Error] datagram_socket::bind : " << ec.message();
        }

        if(socket->set_option(asio::socket_base::broadcast(true), ec))
        {
            log() << "[Error] datagram_socket::broadcast : " << ec.message();
        }
    }

    auto session =
        std::make_shared<udp_connection>(std::move(socket), create_builder, io_context_, heartbeat_);
    session->set_endpoint(endpoint_);

    if(on_connection_ready)
    {
        on_connection_ready(session);
    }

    auto weak_this = weak_ptr(shared_from_this());
    session->on_disconnect.emplace_back([weak_this](connection::id_t, const error_code&) {
        auto shared_this = weak_this.lock();
        if(!shared_this)
        {
            return;
        }
        // Try again.
        shared_this->restart();
    });
}

void basic_client::restart()
{
    using namespace std::chrono_literals;
    reconnect_timer_.expires_from_now(1s);
    reconnect_timer_.async_wait(std::bind(&basic_client::start, shared_from_this()));
}
}
} // namespace net
