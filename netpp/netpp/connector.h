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

struct connector
{
	using id_t = uint64_t;
	using on_connection_ready_t = std::function<void(connection_ptr)>;

	connector();
	virtual ~connector() = default;

	//-----------------------------------------------------------------------------
	/// Starts the connector.
	//-----------------------------------------------------------------------------
	virtual void start() = 0;

	/// callback for when there is a connection ready
	on_connection_ready_t on_connection_ready;

	/// connector id
	const id_t id;
};

using connector_ptr = std::shared_ptr<connector>;
using connector_weak_ptr = std::weak_ptr<connector>;

} // namespace net
