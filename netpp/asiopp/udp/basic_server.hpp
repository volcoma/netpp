#pragma once
#include "../connection.hpp"

#include "datagram_socket.h"
#include <netpp/connector.h>

#include <asio/basic_socket_acceptor.hpp>

namespace net
{
namespace udp
{
template <typename protocol_type>
class basic_server : public connector, public std::enable_shared_from_this<basic_server<protocol_type>>
{
public:
	using protocol = protocol_type;
	using weak_ptr = std::weak_ptr<basic_server<protocol_type>>;
	using protocol_acceptor = asio::basic_socket_acceptor<protocol_type>;
	using protocol_endpoint = typename protocol_type::endpoint;
	using protocol_socket = datagram_socket<protocol_type, true, false>;

	basic_server(asio::io_service& io_context, const protocol_endpoint& endpoint)
		: io_context_(io_context)
        , endpoint_(endpoint)
	{

	}

	void start() override
	{
		auto socket = std::make_shared<protocol_socket>(io_context_, endpoint_);
        socket->set_option(asio::socket_base::reuse_address(true));
        on_handshake_complete(socket);
	}

    template <typename socket_type>
	void on_handshake_complete(const std::shared_ptr<socket_type>& socket)
	{
		//log() << "[NET] : Handshake server::" << socket->lowest_layer().local_endpoint()
		//	  << " -> server::" << socket->lowest_layer().remote_endpoint() << " completed.\n";

		auto session = std::make_shared<net::detail::async_connection<socket_type>>(socket, io_context_);

		if(on_connection_ready)
		{
			on_connection_ready(session);
		}
	}
protected:

    protocol_endpoint endpoint_;
	asio::io_service& io_context_;
};
}
} // namespace net
