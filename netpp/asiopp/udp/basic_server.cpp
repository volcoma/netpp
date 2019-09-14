#include "basic_server.h"
#include "connection.h"
#include <asio/ip/multicast.hpp>

namespace net
{
namespace udp
{

basic_server::basic_server(asio::io_service& io_context, udp::endpoint endpoint,
						   std::chrono::seconds heartbeat)
	: endpoint_(std::move(endpoint))
	, io_context_(io_context)
	, reconnect_timer_(io_context)
	, heartbeat_(heartbeat)
	, strand_(std::make_shared<asio::io_service::strand>(io_context_))
{
	reconnect_timer_.expires_at(asio::steady_timer::time_point::max());
}

void basic_server::start()
{
	connections_.clear();

	auto socket = std::make_shared<udp::socket>(io_context_);
	udp::endpoint socket_endpoint;

	if(endpoint_.address().is_v4())
	{
		socket_endpoint = udp::endpoint(udp::v4(), endpoint_.port());
	}
	else if(endpoint_.address().is_v6())
	{
		socket_endpoint = udp::endpoint(udp::v6(), endpoint_.port());
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
	if(endpoint_.address().is_multicast())
	{
		if(socket->set_option(asio::ip::multicast::join_group(endpoint_.address()), ec))
		{
			log() << "[Error] datagram_socket::multicast::join_group : " << ec.message();
		}
	}

	async_recieve(socket);
}

void basic_server::on_recv_data(const std::shared_ptr<udp::socket>& socket, std::size_t size)
{
	auto session = map_to_connection(socket, remote_endpoint_);

	if(!session)
	{
		return;
	}

	auto unprocessed_size = input_buffer_.offset + size;
	auto result = session->handle_read({}, input_buffer_.buffer.data(), unprocessed_size);
	if(result < 0)
	{
		return;
	}
	auto processed = static_cast<std::size_t>(result);
	auto unprocessed = unprocessed_size - processed;

	if(processed > unprocessed)
	{
		std::memcpy(input_buffer_.buffer.data(), input_buffer_.buffer.data() + processed, unprocessed);
	}
	else
	{
		byte_buffer range(unprocessed);
		std::memcpy(range.data(), input_buffer_.buffer.data() + processed, unprocessed);
		std::memcpy(input_buffer_.buffer.data(), range.data(), range.size());
	}

	input_buffer_.offset = unprocessed;

	async_recieve(socket);
}

auto basic_server::map_to_connection(const std::shared_ptr<udp::socket>& socket,
									 udp::endpoint remote_endpoint) -> srv_connection_ptr
{
	auto it = connections_.find(remote_endpoint);
	if(it == std::end(connections_))
	{
		auto session =
			std::make_shared<udp_server_connection>(socket, create_builder, io_context_, heartbeat_);
		session->set_endpoint(remote_endpoint);
		session->set_strand(strand_);

		auto result = connections_.emplace(remote_endpoint, session);
		if(!result.second)
		{
			return {};
		}

		if(on_connection_ready)
		{
			on_connection_ready(session);
		}

		auto weak_this = weak_ptr(shared_from_this());
		session->on_disconnect.emplace_back(
			[weak_this, remote_endpoint](connection::id_t, const error_code&) {
				auto shared_this = weak_this.lock();
				if(!shared_this)
				{
					return;
				}

				shared_this->io_context_.post(shared_this->strand_->wrap([shared_this, remote_endpoint]() {
					auto sz = shared_this->connections_.size();
					shared_this->connections_.erase(remote_endpoint);
					auto sz_after = shared_this->connections_.size();
					net::log() << "sz before : " << sz << ", sz after : " << sz_after;
				}));

			});

		it = result.first;
	}

	return it->second;
}

void basic_server::async_recieve(std::shared_ptr<udp::socket> socket)
{
	io_context_.post(
		strand_->wrap([this, shared_this = shared_from_this(), socket = std::move(socket)]() mutable {

			// Start an asynchronous operation to read a certain number of bytes.
			// Here std::bind + shared_from_this is used because of the composite op async_*
			// We want it to operate on valid data until the handler is called.
			socket->async_receive_from(
				asio::buffer(input_buffer_.buffer.data() + input_buffer_.offset,
							 input_buffer_.buffer.size() - input_buffer_.offset),
				remote_endpoint_, // asio::socket_base::message_peek,
				strand_->wrap([shared_this, socket](const error_code& ec, std::size_t size) mutable {
					if(ec)
					{
						socket.reset();

						log() << "Receive error: " << ec.message();
						// Start accepting new connections
						shared_this->restart();
					}
					else
					{
						shared_this->on_recv_data(socket, size);
					}
				}));
		}));
}

void basic_server::restart()
{
	using namespace std::chrono_literals;
	reconnect_timer_.expires_from_now(1s);
	reconnect_timer_.async_wait(strand_->wrap(std::bind(&basic_server::start, shared_from_this())));
}
}
} // namespace net
