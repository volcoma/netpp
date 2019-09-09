#include "basic_sender.h"
#include "connection.h"
#include <asio/ip/multicast.hpp>
namespace net
{
namespace udp
{

basic_sender::basic_sender(asio::io_service& io_context, udp::endpoint endpoint)
	: endpoint_(std::move(endpoint))
	, io_context_(io_context)
	, reconnect_timer_(io_context)
{
	reconnect_timer_.expires_at(asio::steady_timer::time_point::max());
}

void basic_sender::start()
{
	auto socket = std::make_shared<udp::socket>(io_context_);

	asio::error_code ec;

	if(socket->open(endpoint_.protocol(), ec))
	{
		log() << "[Error] datagram_socket::open : " << ec.message();
	}
	if(socket->set_option(asio::socket_base::reuse_address(true), ec))
	{
		log() << "[Error] datagram_socket::reuse_address : " << ec.message();
	}

	if(socket->set_option(asio::socket_base::broadcast(true), ec))
	{
		log() << "[Error] datagram_socket::broadcast : " << ec.message();
	}
	//	if(socket->set_option(asio::ip::multicast::enable_loopback(false), ec))
	//	{
	//		log() << "[Error] datagram_socket::multicast::enable_loopback : " << ec.message();
	//	}
	if(socket->set_option(asio::ip::multicast::hops(5), ec))
	{
		log() << "[Error] datagram_socket::multicast::hops : " << ec.message();
	}

	auto session = std::make_shared<udp_connection>(std::move(socket), create_builder, io_context_);
	session->set_endpoint(endpoint_, false, true);

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

	if(on_connection_ready)
	{
		on_connection_ready(session);
	}
}

void basic_sender::restart()
{
	using namespace std::chrono_literals;
	reconnect_timer_.expires_from_now(1s);
	reconnect_timer_.async_wait(std::bind(&basic_sender::start, shared_from_this()));
}
} // namespace udp
} // namespace net
