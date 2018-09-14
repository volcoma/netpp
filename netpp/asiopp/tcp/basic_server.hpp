#pragma once
#include "../connector.h"
#include "connection.hpp"

#include <netpp/msg_builder.hpp>

#include <asio/basic_socket_acceptor.hpp>
#include <asio/basic_stream_socket.hpp>

namespace net
{
namespace tcp
{
template <typename protocol_type>
class basic_server : public asio_connector, public std::enable_shared_from_this<basic_server<protocol_type>>
{
public:
	using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<basic_server<protocol_type>>;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_socket = typename protocol_type::socket;

	basic_server(asio::io_context& io_context, const protocol_endpoint& endpoint)
		: asio_connector(io_context)
		, acceptor_(io_context)
	{
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(asio::socket_base::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();
	}

	void start() override
	{
		using socket_type = protocol_socket;
		auto socket = std::make_shared<socket_type>(io_context_);

		auto weak_this = weak_ptr(this->shared_from_this());
		auto on_connection_established = [weak_this, socket]() {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->on_handshake_complete(socket);
		};

		async_accept(socket, std::move(on_connection_established));
	}

	template <typename socket_type, typename F>
	void async_accept(std::shared_ptr<socket_type> socket, F f)
	{
		sout() << "accepting connections on " << acceptor_.local_endpoint() << "..."
			   << "\n";

		auto weak_this = weak_ptr(this->shared_from_this());
		acceptor_.async_accept(socket->lowest_layer(),
							   [weak_this, socket, on_connection_established = std::move(f)](
								   const asio::error_code& ec) mutable {
								   if(ec)
								   {
									   sout() << "accept error: " << ec.message() << "\n";

									   // We need to close the socket used in the previous connection attempt
									   // before starting a new one.
									   socket.reset();
								   }
								   // Otherwise we have successfully established a connection.
								   else
								   {
									   on_connection_established();
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

	template <typename socket_type>
	void on_handshake_complete(std::shared_ptr<socket_type> socket)
	{
		sout() << "handshake " << socket->lowest_layer().local_endpoint() << " -> "
			   << socket->lowest_layer().remote_endpoint() << " completed"
			   << "\n";

		using builder_type = msg_builder<uint32_t>;
		auto session = std::make_shared<async_connection<socket_type, builder_type>>(socket, io_context_);
		auto weak_this = weak_ptr(this->shared_from_this());

		session->on_connect = [weak_this](connection_ptr session) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->add(session);
			shared_this->on_connect(session->id);
		};

		session->on_disconnect = [weak_this](connection_ptr session, const error_code& ec) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->remove(session);
			shared_this->on_disconnect(session->id, ec);
		};

		session->on_msg = [weak_this](connection_ptr session, const byte_buffer& msg) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->on_msg(session->id, msg);
		};

		session->start();
	}

protected:
	protocol_acceptor acceptor_;
};
}
} // namespace net
