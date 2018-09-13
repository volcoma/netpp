#pragma once

#include "tcp_connector.hpp"

#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
namespace net
{
using asio::local::stream_protocol;

struct tcp_local_client : tcp_basic_client<stream_protocol>
{
	using tcp_basic_client<stream_protocol>::tcp_basic_client;
	void start() override;
};
} // namespace net
#endif
