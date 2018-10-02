#include "basic_reciever.h"
#include "connection.h"
#include <asio/ip/multicast.hpp>

namespace net
{
namespace udp
{

basic_reciever::basic_reciever(asio::io_service& io_context, udp::endpoint endpoint)
	: endpoint_(std::move(endpoint))
	, io_context_(io_context)
	, reconnect_timer_(io_context)
{
	reconnect_timer_.expires_at(asio::steady_timer::time_point::max());
}

void basic_reciever::start()
{
	auto socket = std::make_shared<udp::socket>(io_context_);

	udp::endpoint listen_endpoint;
	if(endpoint_.address().is_multicast())
	{
		if(endpoint_.address().is_v4())
		{
			listen_endpoint = udp::endpoint(asio::ip::address_v4::any(), endpoint_.port());
		}
		else if(endpoint_.address().is_v6())
		{
			listen_endpoint = udp::endpoint(asio::ip::address_v6::any(), endpoint_.port());
		}
	}
	else
	{
		listen_endpoint = endpoint_;
	}
	asio::error_code ec;
	if(socket->open(listen_endpoint.protocol(), ec))
	{
		log() << "[Error] datagram_socket::open : " << ec.message();
	}
	if(socket->set_option(asio::socket_base::reuse_address(true), ec))
	{
		log() << "[Error] datagram_socket::reuse_address : " << ec.message();
	}

	if(socket->bind(listen_endpoint, ec))
	{
		log() << "[Error] datagram_socket::bind : " << ec.message();
	}
	if(endpoint_.address().is_multicast())
	{
		if(socket->set_option(asio::ip::multicast::join_group(endpoint_.address()), ec))
		{
			log() << "[Error] datagram_socket::multicast::join_group : " << ec.message();
		}
	}

	auto session = std::make_shared<udp_connection>(std::move(socket), create_builder, io_context_);
	session->set_endpoint(listen_endpoint, true, false);

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

void basic_reciever::restart()
{
	using namespace std::chrono_literals;
	reconnect_timer_.expires_from_now(1s);
	reconnect_timer_.async_wait(std::bind(&basic_reciever::start, shared_from_this()));
}
}
} // namespace net
