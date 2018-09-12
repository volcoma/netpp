#pragma once
#include "tcp_connector.hpp"

#include <asio/ip/tcp.hpp>
namespace net
{
using asio::ip::tcp;

struct tcp_server : tcp_basic_server<tcp>
{
	using tcp_basic_server<tcp>::tcp_basic_server;
	void start() override;
};
} // namespace net
