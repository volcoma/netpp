#pragma once
#include "connection.h"

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <chrono>
#include <deque>
#include <thread>

namespace net
{
namespace udp
{

using asio::ip::udp;

class udp_server_connection : public udp_connection
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using base_type = udp_connection;

	//-----------------------------------------------------------------------------
	/// Inherited constructors
	//-----------------------------------------------------------------------------
	using base_type::base_type;

	//-----------------------------------------------------------------------------
	/// Sets and endpoint and read/write rights
	//-----------------------------------------------------------------------------
	void set_strand(std::shared_ptr<asio::io_service::strand> strand);

	//-----------------------------------------------------------------------------
	/// Starts the async read operation awaiting for data
	/// to be read from the socket.
	//-----------------------------------------------------------------------------
	void start_read() override;

private:
	void stop_socket() override;

	/// Endpoint used for sending
	udp::endpoint endpoint_;
};
}
} // namespace net
