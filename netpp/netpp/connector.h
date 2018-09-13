#pragma once
#include "connection.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <functional>
#include <string>
#include <thread>

namespace net
{

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

    void add(const connection_ptr& session);
	void remove(const connection_ptr& session);

	on_connect_t on_connect;
	on_disconnect_t on_disconnect;
	on_msg_t on_msg;

    std::mutex guard_;
	std::map<connection::id_t, connection_ptr> connections_;

	const id_t id;
};

using connector_ptr = std::shared_ptr<connector>;
using connector_weak_ptr = std::weak_ptr<connector>;

} // namespace net
