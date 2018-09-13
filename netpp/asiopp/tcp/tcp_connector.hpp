#pragma once
#include "../connector.h"
#include "tcp_connection.hpp"

#include <netpp/msg_builder.hpp>

#include <asio/basic_socket_acceptor.hpp>
#include <asio/basic_stream_socket.hpp>
#include <asio/ip/basic_endpoint.hpp>

#include <iostream>
namespace net
{

template <typename protocol_type>
class tcp_basic_client : public asio_connector,
						 public std::enable_shared_from_this<tcp_basic_client<protocol_type>>
{
public:
    using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<tcp_basic_client<protocol_type>>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_acceptor_ptr = std::unique_ptr<protocol_acceptor>;

	tcp_basic_client(asio::io_context& io_context, const protocol_endpoint& endpoint)
		: asio_connector(io_context)
		, endpoint_(endpoint)
	{
	}

	template <typename socket_type, typename F>
	void async_connect(std::shared_ptr<socket_type> socket, F f)
	{
		std::cout << "trying to connect to " << endpoint_ << "..." << std::endl;

		auto weak_this = weak_ptr(this->shared_from_this());

		socket->lowest_layer().async_connect(
			endpoint_,
			[weak_this, socket, on_connection_established = std::move(f)](const error_code& ec) mutable {
				// The async_connect() function automatically opens the socket at the start
				// of the asynchronous operation.

				// Check if the connect operation failed before the deadline expired.
				if(ec)
				{
					// std::cout << "Connect error: " << ec.message() << std::endl;
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
		std::cout << "handshake " << socket->lowest_layer().local_endpoint() << " -> "
				  << socket->lowest_layer().remote_endpoint() << " completed" << std::endl;

		using builder_type = msg_builder<uint32_t>;
		auto session = std::make_shared<tcp_connection<socket_type, builder_type>>(socket, io_context);
		auto weak_this = weak_ptr(this->shared_from_this());

		session->on_connect = [weak_this](connection_ptr session) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->channel.join(session);
			shared_this->on_connect(session->id);
		};

		session->on_disconnect = [weak_this](connection_ptr session, const error_code& ec) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->channel.leave(session);
			shared_this->on_disconnect(session->id, ec);

			// Try again
			shared_this->start();
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
	protocol_endpoint endpoint_;
};

template <typename protocol_type>
class tcp_basic_server : public asio_connector,
						 public std::enable_shared_from_this<tcp_basic_server<protocol_type>>
{
public:
    using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<tcp_basic_server<protocol_type>>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_acceptor_ptr = std::unique_ptr<protocol_acceptor>;

	tcp_basic_server(asio::io_context& io_context, const protocol_endpoint& endpoint)
		: asio_connector(io_context)
		, acceptor_(io_context, endpoint)
	{
		acceptor_.set_option(asio::socket_base::reuse_address(true));
	}

	template <typename socket_type, typename F>
	void async_accept(std::shared_ptr<socket_type> socket, F f)
	{
		std::cout << "accepting connections on " << acceptor_.local_endpoint() << "..." << std::endl;

		auto weak_this = weak_ptr(this->shared_from_this());
		acceptor_.async_accept(socket->lowest_layer(),
							   [weak_this, socket, on_connection_established = std::move(f)](
								   const asio::error_code& ec) mutable {
								   if(ec)
								   {
									   std::cout << "accept error: " << ec.message() << std::endl;

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
		std::cout << "handshake " << socket->lowest_layer().local_endpoint() << " -> "
				  << socket->lowest_layer().remote_endpoint() << " completed" << std::endl;

		using builder_type = msg_builder<uint32_t>;
		auto session = std::make_shared<tcp_connection<socket_type, builder_type>>(socket, io_context);
		auto weak_this = weak_ptr(this->shared_from_this());

		session->on_connect = [weak_this](connection_ptr session) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->channel.join(session);
			shared_this->on_connect(session->id);
		};

		session->on_disconnect = [weak_this](connection_ptr session, const error_code& ec) mutable {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			shared_this->channel.leave(session);
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

} // namespace net
