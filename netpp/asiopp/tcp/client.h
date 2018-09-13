#pragma once
#include "basic_client.hpp"
#include "ssl_basic_client.hpp"

#include <asio/ip/tcp.hpp>

#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
#endif
namespace net
{
namespace tcp
{

using client = basic_client<asio::ip::tcp>;
using ssl_client = ssl_basic_client<asio::ip::tcp>;

#ifdef ASIO_HAS_LOCAL_SOCKETS
using local_client = basic_client<asio::local::stream_protocol>;
using ssl_local_client = ssl_basic_client<asio::local::stream_protocol>;
#endif
}

} // namespace net
