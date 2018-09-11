#pragma once
#include "connector.h"

#include <asio/ip/tcp.hpp>
#include <asio/ssl.hpp>
namespace net
{

//----------------------------------------------------------------------
using asio::ip::tcp;
class tcp_ssl_server : public connector
{
public:
	tcp_ssl_server(asio::io_context& io_context, const tcp::endpoint& listen_endpoint,
				   const std::string& cert_chain_file, const std::string& private_key_file,
				   const std::string& dh_file);
	void start() override;

private:

	std::string get_password() const;
	tcp::acceptor acceptor_;
	asio::ssl::context context_;
};
} // namespace net
