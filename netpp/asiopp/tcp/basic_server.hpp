#pragma once
#include "connection.hpp"

#include <netpp/connector.h>

#include <asio/basic_socket_acceptor.hpp>

namespace net
{
namespace tcp
{
template <typename protocol_type>
class basic_server : public connector, public std::enable_shared_from_this<basic_server<protocol_type>>
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<basic_server<protocol_type>>;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_socket = typename protocol_type::socket;

	//-----------------------------------------------------------------------------
	/// Constructor of client accepting a connect endpoint.
	//-----------------------------------------------------------------------------
	basic_server(asio::io_service& io_context, const protocol_endpoint& listen_endpoint,
				 std::chrono::seconds heartbeat = std::chrono::seconds{0});

	//-----------------------------------------------------------------------------
	/// Starts the server attempting to accept incomming connections.
	//-----------------------------------------------------------------------------
	void start() override;
	void restart();

	template <typename socket_type, typename F>
	void async_accept(socket_type& socket, F f);

	template <typename socket_type>
	void on_handshake_complete(std::shared_ptr<socket_type> socket);

protected:
	protocol_acceptor acceptor_;
	protocol_endpoint endpoint_;
	asio::steady_timer reconnect_timer_;
	asio::io_service& io_context_;
	std::chrono::seconds heartbeat_;
};

template <typename protocol_type>
inline basic_server<protocol_type>::basic_server(asio::io_service& io_context,
												 const protocol_endpoint& listen_endpoint,
												 std::chrono::seconds heartbeat)
	: acceptor_(io_context)
	, endpoint_(listen_endpoint)
	, reconnect_timer_(io_context)
	, io_context_(io_context)
	, heartbeat_(heartbeat)
{
	reconnect_timer_.expires_at(asio::steady_timer::time_point::max());

	acceptor_.open(endpoint_.protocol());
	acceptor_.set_option(asio::socket_base::reuse_address(true));
	acceptor_.bind(endpoint_);
	acceptor_.listen();
}

template <typename protocol_type>
inline void basic_server<protocol_type>::start()
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

	async_accept(socket, std::move(on_connection_established));
}

template <typename protocol_type>
inline void basic_server<protocol_type>::restart()
{
	using namespace std::chrono_literals;
	reconnect_timer_.expires_after(1s);
	reconnect_timer_.async_wait(std::bind(&basic_server::start, this->shared_from_this()));
}

template <typename protocol_type>
template <typename socket_type, typename F>
inline void basic_server<protocol_type>::async_accept(socket_type& socket, F f)
{
	log() << "Accepting connections on " << acceptor_.local_endpoint() << "...";

	auto weak_this = weak_ptr(this->shared_from_this());
	auto& lowest_layer = socket->lowest_layer();
	acceptor_.async_accept(lowest_layer,
						   [weak_this, socket = std::move(socket),
							on_connection_established = std::move(f)](const error_code& ec) mutable {
							   if(ec)
							   {
								   log() << "Accept error: " << ec.message();

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
							   // Start accepting new connections
							   shared_this->start();
						   });
}

template <typename protocol_type>
template <typename socket_type>
inline void basic_server<protocol_type>::on_handshake_complete(std::shared_ptr<socket_type> socket)
{
	log() << "Handshake server::" << socket->lowest_layer().local_endpoint()
		  << " -> client::" << socket->lowest_layer().remote_endpoint() << " completed.";

	auto session =
		std::make_shared<tcp_connection<socket_type>>(socket, create_builder, io_context_, heartbeat_);
	if(on_connection_ready)
	{
		on_connection_ready(session);
	}
}
}
} // namespace net
