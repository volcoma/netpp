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
	using id_t = uint64_t;
	using on_connect_t = std::function<void(connection::id_t)>;
	using on_disconnect_t = std::function<void(connection::id_t, const error_code&)>;
	using on_msg_t = std::function<void(connection::id_t, const byte_buffer&)>;

	connection();
	virtual ~connection() = default;
	virtual void send_msg(byte_buffer&& msg, data_channel channel) = 0;
	virtual void start() = 0;
	virtual void stop(const error_code& ec) = 0;

	std::vector<on_disconnect_t> on_disconnect;
	on_msg_t on_msg;
	msg_builder_ptr msg_builder_;
	const id_t id;
};
using connection_ptr = std::shared_ptr<connection>;

} // namespace net
