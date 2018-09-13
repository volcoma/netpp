#pragma once
#include <netpp/connector.h>

#include <chrono>
#include <memory>
#include <thread>
namespace net
{

void init_services(size_t workers);
void deinit_services();

net::connector_ptr create_tcp_server(uint16_t port);
net::connector_ptr create_tcp_client(const std::string& host, const std::string& port);

net::connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
										 const std::string& private_key_file, const std::string& dh_file);

net::connector_ptr create_tcp_ssl_client(const std::string& host, const std::string& port,
										 const std::string& cert_file);
net::connector_ptr create_tcp_local_server(const std::string& file);
net::connector_ptr create_tcp_local_client(const std::string& file);

} // namespace net
