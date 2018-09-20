#pragma once
#include "../common/connection.hpp"

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

class udp_connection : public asio_connection<udp::socket>
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using base_type = asio_connection<udp::socket>;

	//-----------------------------------------------------------------------------
	/// Inherited constructors
	//-----------------------------------------------------------------------------
	using base_type::base_type;

	//-----------------------------------------------------------------------------
	/// Sets and endpoint and read/write rights
	//-----------------------------------------------------------------------------
	void set_endpoint(udp::endpoint endpoint, bool can_read, bool can_write);

	//-----------------------------------------------------------------------------
	/// Starts the async read operation awaiting for data
	/// to be read from the socket.
	//-----------------------------------------------------------------------------
	void start_read() override;

	//-----------------------------------------------------------------------------
	/// Callback to be called whenever data was read from the socket
	/// or an error occured.
	//-----------------------------------------------------------------------------
	void handle_read(const error_code& ec, std::size_t n) override;

	//-----------------------------------------------------------------------------
	/// Starts the async write operation awaiting for data
	/// to be written to the socket.
	//-----------------------------------------------------------------------------
	void start_write() override;

	//-----------------------------------------------------------------------------
	/// Callback to be called whenever data was written to the socket
	/// or an error occured.
	//-----------------------------------------------------------------------------
	void handle_write(const error_code& ec) override;

private:
	/// Endpoint used for sending
	udp::endpoint endpoint_;

	/// Endpoint filled when receiving
	udp::endpoint remote_endpoint_;

	/// a flag indicating if this connection can read
	bool can_read_ = true;

	/// a flag indicating if this connection can write
	bool can_write_ = true;
};
}
} // namespace net