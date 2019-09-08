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

class basic_reciever : public connector, public std::enable_shared_from_this<basic_reciever>
{
public:
	using weak_ptr = std::weak_ptr<basic_reciever>;
	~basic_reciever() override = default;
	//-----------------------------------------------------------------------------
	/// Constructor of client accepting a receive endpoint.
	//-----------------------------------------------------------------------------
	basic_reciever(asio::io_service& io_context, udp::endpoint endpoint);

	//-----------------------------------------------------------------------------
	/// Starts the receiver creating an udp socket and setting proper options
	/// depending on the type of the endpoint
	//-----------------------------------------------------------------------------
	void start() override;

protected:
	void restart();
	udp::endpoint endpoint_;
	asio::io_service& io_context_;
	asio::steady_timer reconnect_timer_;
};
} // namespace udp
} // namespace net
