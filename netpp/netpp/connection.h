#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace net
{
using byte_buffer = std::string; // std::vector<uint8_t>;
using error_code = std::error_code;
//----------------------------------------------------------------------
struct connection;
using connection_ptr = std::shared_ptr<connection>;

struct connection
{
	using on_connect_t = std::function<void(connection_ptr)>;
	using on_disconnect_t = std::function<void(connection_ptr, const error_code&)>;
	using on_msg_t = std::function<void(connection_ptr, const byte_buffer&)>;
	using id_t = uint64_t;

	connection();
	virtual ~connection() = default;
	virtual void send_msg(byte_buffer&&) = 0;
	virtual void start() = 0;
	virtual void stop(const error_code& ec) = 0;

	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_msg_t on_msg;
	const id_t id;
};

} // namespace net
