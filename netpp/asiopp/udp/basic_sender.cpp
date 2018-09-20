#include "basic_sender.h"
#include <chrono>
#include <thread>
namespace net
{
namespace udp
{

basic_sender::basic_sender(asio::io_service& io_context, udp::endpoint endpoint)
	: endpoint_(std::move(endpoint))
	, io_context_(io_context)
{
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
	// sock is now open. Any failure in this scope will require a close()
	if(socket->set_option(asio::ip::multicast::enable_loopback(false), ec))
	{
		log() << "[Error] datagram_socket::multicast::enable_loopback : " << ec.message();
	}

	auto session = std::make_shared<async_connection>(std::move(socket), endpoint_, io_context_, false, true);

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
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(500ms);
		// Try again.
		shared_this->start();
	});
}
}
} // namespace net
