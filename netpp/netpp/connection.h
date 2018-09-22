#pragma once

#include "msg_builder.h"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace net
{
using error_code = std::error_code;
//----------------------------------------------------------------------

struct connection
{
	//-----------------------------------------------------------------------------
	/// Aliases
	//-----------------------------------------------------------------------------
	using id_t = uint64_t;
	using on_disconnect_t = std::function<void(connection::id_t, const error_code&)>;
	using on_msg_t = std::function<void(connection::id_t, byte_buffer)>;

	connection();
	virtual ~connection() = default;

	//-----------------------------------------------------------------------------
	/// Sends the message through the specified channel
	//-----------------------------------------------------------------------------
	virtual void send_msg(byte_buffer&& msg, data_channel channel) = 0;

	//-----------------------------------------------------------------------------
	/// Starts the connection.
	//-----------------------------------------------------------------------------
	virtual void start() = 0;

	//-----------------------------------------------------------------------------
	/// Stops the connection with the specified error code.
	//-----------------------------------------------------------------------------
	virtual void stop(const error_code& ec) = 0;

	/// container of subscribers for on_msg
	std::vector<on_msg_t> on_msg;

	/// container of subscribers for on_disconnect
	std::vector<on_disconnect_t> on_disconnect;

	/// unique msg_builder for this connection
	msg_builder_ptr builder;

	/// connection id
	const id_t id;
};
using connection_ptr = std::shared_ptr<connection>;

} // namespace net
