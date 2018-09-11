#pragma once
#include "connector.h"

#include <asio/local/stream_protocol.hpp>

#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{

//----------------------------------------------------------------------
using asio::local::stream_protocol;

class local_server : public connector
{
public:
	local_server(asio::io_context& io_context, const stream_protocol::endpoint& listen_endpoint);
	void start() override;

private:
	stream_protocol::acceptor acceptor_;
};
} // namespace net
#endif
