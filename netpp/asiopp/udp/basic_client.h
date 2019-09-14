#pragma once
#include <netpp/connector.h>

#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>

namespace net
{
namespace udp
{

using asio::ip::udp;

class basic_client : public connector, public std::enable_shared_from_this<basic_client>
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using weak_ptr = std::weak_ptr<basic_client>;

	//-----------------------------------------------------------------------------
	/// Constructor of client accepting an endpoint.
	//-----------------------------------------------------------------------------
	basic_client(asio::io_service& io_context, udp::endpoint endpoint,
				 std::chrono::seconds heartbeat = std::chrono::seconds{0});

	//-----------------------------------------------------------------------------
	/// Starts the client creating an udp socket and setting proper options
	/// depending on the type of the endpoint
	//-----------------------------------------------------------------------------
	void start() override;
	void restart();

protected:
	udp::endpoint endpoint_;
	asio::io_service& io_context_;
	asio::steady_timer reconnect_timer_;
	std::chrono::seconds heartbeat_;
};
}
} // namespace net
