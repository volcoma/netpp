#pragma once
#include "server_connection.h"
#include <netpp/connector.h>

#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>

namespace net
{
namespace udp
{

using asio::ip::udp;

class basic_server : public connector, public std::enable_shared_from_this<basic_server>
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using weak_ptr = std::weak_ptr<basic_server>;
	using srv_connection_ptr = std::shared_ptr<udp_server_connection>;

	//-----------------------------------------------------------------------------
	/// Constructor of client accepting an endpoint.
	//-----------------------------------------------------------------------------
	basic_server(asio::io_service& io_context, udp::endpoint endpoint,
				 std::chrono::seconds heartbeat = std::chrono::seconds{0});

	//-----------------------------------------------------------------------------
	/// Starts the receiver creating an udp socket and setting proper options
	/// depending on the type of the endpoint
	//-----------------------------------------------------------------------------
	void start() override;
	void restart();

protected:
	void async_recieve(std::shared_ptr<udp::socket> socket);
	void on_recv_data(const std::shared_ptr<udp::socket>& socket, std::size_t size);
	auto map_to_connection(const std::shared_ptr<udp::socket>& socket, udp::endpoint remote_endpoint)
		-> srv_connection_ptr;

	udp::endpoint endpoint_;
	asio::io_service& io_context_;
	asio::steady_timer reconnect_timer_;
	std::chrono::seconds heartbeat_;

	input_buffer input_buffer_;
	udp::endpoint remote_endpoint_;

	std::shared_ptr<asio::io_service::strand> strand_;
	std::map<udp::endpoint, srv_connection_ptr> connections_;
};
}
} // namespace net
