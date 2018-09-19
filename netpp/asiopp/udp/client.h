#pragma once
#include "basic_client.hpp"

#include <asio/ip/udp.hpp>
namespace net
{
namespace udp
{
using client = basic_client<asio::ip::udp>;

}
}

