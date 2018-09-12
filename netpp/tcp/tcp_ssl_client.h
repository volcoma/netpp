#pragma once

#include "tcp_client.h"
#include <asio/ssl.hpp>

namespace net
{

using asio::ip::tcp;

class tcp_ssl_client : public tcp_client
{
public:
	tcp_ssl_client(asio::io_context& io_context, const protocol_endpoint& endpoint,
				   const std::string& cert_file);

	void start() override;

private:

	asio::ssl::context context_;
};
} // namespace net
