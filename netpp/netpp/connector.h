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
	using on_connection_ready_t = std::function<void(connection_ptr)>;
	connector();
	virtual ~connector() = default;
	virtual void start() = 0;

	on_connection_ready_t on_connection_ready;
	const id_t id;
};

using connector_ptr = std::shared_ptr<connector>;
using connector_weak_ptr = std::weak_ptr<connector>;

} // namespace net
