#pragma once

#include "tcp_local_client.h"
#include <asio/ssl.hpp>
#ifdef ASIO_HAS_LOCAL_SOCKETS
namespace net
{

class tcp_ssl_local_client : public tcp_local_client
{
public:
	tcp_ssl_local_client(asio::io_context& io_context, const protocol_endpoint& endpoint,
				   const std::string& cert_file);

	void start() override;

private:
	asio::ssl::context context_;
};
} // namespace net
#endif
