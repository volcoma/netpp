#pragma once
#include "basic_server.hpp"

#include <asio/ip/udp.hpp>
namespace net
{
namespace udp
{
using server = basic_server<asio::ip::udp>;

}
}
