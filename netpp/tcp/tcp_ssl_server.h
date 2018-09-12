#pragma once
#include "tcp_server.h"

#include <asio/ssl.hpp>
namespace net
{

//----------------------------------------------------------------------
class tcp_ssl_server : public tcp_server
{
public:
	tcp_ssl_server(asio::io_context& io_context, const protocol_endpoint& listen_endpoint,
				   const std::string& cert_chain_file, const std::string& private_key_file,
				   const std::string& dh_file);
	void start() override;

private:
	std::string get_password() const;
	asio::ssl::context context_;
};
} // namespace net
