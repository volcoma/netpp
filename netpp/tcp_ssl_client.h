#pragma once

#include "connector.h"
#include <asio/ip/tcp.hpp>
#include <asio/ssl.hpp>

namespace net
{

using asio::ip::tcp;

class tcp_ssl_client : public connector
{
public:
	tcp_ssl_client(asio::io_context& io_context, tcp::endpoint endpoint,
				   const std::string& cert_file);

	void start() override;

private:

	tcp::endpoint endpoint_;
	asio::ssl::context context_;
};
} // namespace net
