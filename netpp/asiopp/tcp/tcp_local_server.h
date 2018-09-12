#pragma once
#include "tcp_connector.hpp"

#include <asio/local/stream_protocol.hpp>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{
using asio::local::stream_protocol;

struct tcp_local_server : tcp_basic_server<stream_protocol>
{
	using tcp_basic_server<stream_protocol>::tcp_basic_server;
	void start() override;
};
} // namespace net
#endif
