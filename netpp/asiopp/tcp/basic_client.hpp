#pragma once
#include "connection.hpp"

#include <netpp/connector.h>
#include <asio/io_context.hpp>
#include <asio/basic_socket_acceptor.hpp>

namespace net
{
namespace tcp
{

template <typename protocol_type>
class basic_client : public connector, public std::enable_shared_from_this<basic_client<protocol_type>>
{
public:
	using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<basic_client<protocol_type>>;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_socket = typename protocol_type::socket;

	basic_client(asio::io_context& io_context, const protocol_endpoint& endpoint)
		: io_context_(io_context)
		, endpoint_(endpoint)
	{
	}

	void start() override
	{
		auto socket = std::make_shared<protocol_socket>(io_context_);

		auto weak_this = weak_ptr(this->shared_from_this());
		auto on_connection_established = [weak_this, socket]() {
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
	void async_connect(socket_type& socket, F f)
	{
		log() << "[NET] : Trying to connect to " << endpoint_ << " ...\n";

		auto weak_this = weak_ptr(this->shared_from_this());

		auto& lowest_layer = socket->lowest_layer();

		lowest_layer.async_connect(
			endpoint_, [weak_this, socket = std::move(socket),
						on_connection_established = std::move(f)](const error_code& ec) mutable {
				// The async_connect() function automatically opens the socket at the start
				// of the asynchronous operation.

				// Check if the connect operation failed before the deadline expired.
				if(ec)
				{
					// log() << "[NET] : Connect error: " << ec.message() << "\n";
					socket.reset();

					auto shared_this = weak_this.lock();
					if(!shared_this)
					{
						return;
					}

					// Try to connect again.
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
	void on_handshake_complete(const std::shared_ptr<socket_type>& socket)
	{
		log() << "[NET] : Handshake client::" << socket->lowest_layer().local_endpoint()
			  << " -> server::" << socket->lowest_layer().remote_endpoint() << " completed.\n";

		auto session = std::make_shared<async_connection<socket_type>>(socket, io_context_);

		if(on_connection_ready)
		{
			on_connection_ready(session);
		}

		auto weak_this = weak_ptr(this->shared_from_this());
		session->on_disconnect.emplace_back([weak_this](connection::id_t, const error_code&) {
			auto shared_this = weak_this.lock();
			if(!shared_this)
			{
				return;
			}
			// Try again.
			shared_this->start();
		});
	}

protected:
	protocol_endpoint endpoint_;
    asio::io_context& io_context_;
};
}
} // namespace net
