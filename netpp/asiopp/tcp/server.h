#pragma once
#include "basic_server.hpp"
#include "basic_ssl_server.hpp"

#include <asio/ip/tcp.hpp>
namespace net
{
namespace tcp
{
using server = basic_server<asio::ip::tcp>;
using ssl_server = basic_ssl_server<asio::ip::tcp>;
}
}

#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
namespace net
{
namespace tcp
{
using local_server = basic_server<asio::local::stream_protocol>;
using ssl_local_server = basic_ssl_server<asio::local::stream_protocol>;
}
} // namespace net
#endif
