#pragma once
#include "channel.h"

#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/steady_timer.hpp>
#include <asio/write.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <thread>

namespace net
{

//----------------------------------------------------------------------

struct connector : public std::enable_shared_from_this<connector>
{
	using id = uint64_t;
	using on_msg_t = std::function<void(connection::id, const byte_buffer&)>;
	using on_connect_t = std::function<void(connection::id)>;
	using on_disconnect_t = std::function<void(connection::id, asio::error_code)>;

	connector(asio::io_context& io_context);
	virtual ~connector() = default;
	virtual void start() = 0;

	void stop();
    void stop(connection::id id);

	void send_msg(connection::id id, byte_buffer&& msg);
	void on_msg(connection::id id, const byte_buffer& msg);
	void on_disconnect(connection::id id, asio::error_code ec);
	void on_connect(connection::id id);

    void setup_connection(connection& session);

	const id& get_id() const;

	id id_;
	asio::io_context& io_context_;
	channel channel_;

	on_connect_t on_connect_;
	on_disconnect_t on_disconnect_;
	on_msg_t on_msg_;
};

using connector_ptr = std::shared_ptr<connector>;
using connector_weak_ptr = std::weak_ptr<connector>;

} // namespace net
