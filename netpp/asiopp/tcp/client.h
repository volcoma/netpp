#pragma once
#include "basic_client.hpp"
#include "basic_ssl_client.hpp"

#include <asio/ip/tcp.hpp>
namespace net
{
namespace tcp
{
using client = basic_client<asio::ip::tcp>;

namespace ssl
{
using client = basic_ssl_client<asio::ip::tcp>;
}
}
}

#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
namespace net
{
namespace tcp
{
using local_client = basic_client<asio::local::stream_protocol>;
namespace ssl
{
using local_client = basic_ssl_client<asio::local::stream_protocol>;
}
}
} // namespace net
#endif
