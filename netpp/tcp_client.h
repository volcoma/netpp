#pragma once

#include "connector.h"
#include <asio/ip/tcp.hpp>

namespace net
{

using asio::ip::tcp;

class tcp_client : public connector
{
public:
	tcp_client(asio::io_context& io_context, tcp::endpoint endpoint);

	void start() override;

protected:
	tcp::endpoint endpoint_;
};
} // namespace net
