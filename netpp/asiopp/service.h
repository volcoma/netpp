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
net::connector_ptr create_tcp_client(const std::string& host, uint16_t port);

net::connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
										 const std::string& private_key_file, const std::string& dh_file);

net::connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port,
										 const std::string& cert_file);

net::connector_ptr create_tcp_local_server(const std::string& file);
net::connector_ptr create_tcp_local_client(const std::string& file);

net::connector_ptr create_tcp_ssl_local_server(const std::string& file, const std::string& cert_chain_file,
											   const std::string& private_key_file,
											   const std::string& dh_file);

net::connector_ptr create_tcp_ssl_local_client(const std::string& file, const std::string& cert_file);

net::connector_ptr create_udp_unicast_server(const std::string& unicast_address, uint16_t port);
net::connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port);

net::connector_ptr create_udp_multicast_server(const std::string& multicast_address, uint16_t port);
net::connector_ptr create_udp_multicast_client(const std::string& multicast_address, uint16_t port);

// IMPORTANT! -> There are no broadcast addresses in IPv6, their function being
// superseded by multicast addresses.
net::connector_ptr create_udp_broadcast_server(uint16_t port);
net::connector_ptr create_udp_broadcast_client(uint16_t port);
} // namespace net
