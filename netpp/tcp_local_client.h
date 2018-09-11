#pragma once

#include "connector.h"
#include <asio/local/stream_protocol.hpp>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{
using asio::local::stream_protocol;

class local_client : public connector
{
public:
	local_client(asio::io_context& io_context, stream_protocol::endpoint endpoint);

	void start() override;

private:
	stream_protocol::endpoint endpoint_;
};
} // namespace net
#endif
