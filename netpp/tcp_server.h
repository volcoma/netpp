#pragma once
#include "connector.h"

#include <asio/ip/tcp.hpp>
namespace net
{

//----------------------------------------------------------------------
using asio::ip::tcp;

class tcp_server : public connector
{
public:
	tcp_server(asio::io_context& io_context, const tcp::endpoint& listen_endpoint);
	void start() override;

private:
	tcp::acceptor acceptor_;
};
} // namespace net
