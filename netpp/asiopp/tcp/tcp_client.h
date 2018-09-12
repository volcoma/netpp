#pragma once
#include "tcp_connector.hpp"

#include <asio/ip/tcp.hpp>
namespace net
{
using asio::ip::tcp;

struct tcp_client : tcp_basic_client<tcp>
{
	using tcp_basic_client<tcp>::tcp_basic_client;
	void start() override;
};
} // namespace net
