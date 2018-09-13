#pragma once
#include "basic_server.hpp"
#include "ssl_basic_server.hpp"

#include <asio/ip/tcp.hpp>

#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
#endif
namespace net
{
namespace tcp
{

using server = basic_server<asio::ip::tcp>;
using ssl_server = ssl_basic_server<asio::ip::tcp>;

#ifdef ASIO_HAS_LOCAL_SOCKETS
using local_server = basic_server<asio::local::stream_protocol>;
using ssl_local_server = ssl_basic_server<asio::local::stream_protocol>;
#endif

}


} // namespace net
