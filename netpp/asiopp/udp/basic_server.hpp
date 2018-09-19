#pragma once
#include "connection.hpp"

#include "datagram_socket.h"
#include <netpp/connector.h>
#include <asio/ip/multicast.hpp>

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
        auto session = std::make_shared<async_connection<protocol_socket>>(socket, io_context_);

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
