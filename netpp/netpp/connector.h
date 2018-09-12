#pragma once
#include "connection.h"

//#include <asio/buffer.hpp>
//#include <asio/io_context.hpp>
//#include <asio/read.hpp>
//#include <asio/read_until.hpp>
//#include <asio/steady_timer.hpp>
//#include <asio/write.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <thread>

namespace net
{

class connections
{
public:
	void join(const connection_ptr& session);
	void leave(const connection_ptr& session);
	void stop(const error_code& ec);
	void stop(connection::id_t id, const error_code& ec);
	void send_msg(connection::id_t id, byte_buffer&& msg);

private:
	std::mutex guard_;
	std::map<connection::id_t, connection_ptr> connections_;
};
//----------------------------------------------------------------------

struct connector
{
	using id_t = uint64_t;
	using on_msg_t = std::function<void(connection::id_t, const byte_buffer&)>;
	using on_connect_t = std::function<void(connection::id_t)>;
	using on_disconnect_t = std::function<void(connection::id_t, const std::error_code&)>;

	connector();
	virtual ~connector() = default;
	virtual void start() = 0;

	void stop(const std::error_code& ec);
	void stop(connection::id_t id, const std::error_code& ec);
	void send_msg(connection::id_t id, byte_buffer&& msg);

	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_msg_t on_msg;

	connections channel;
	const id_t id;
};

using connector_ptr = std::shared_ptr<connector>;
using connector_weak_ptr = std::weak_ptr<connector>;

} // namespace net
