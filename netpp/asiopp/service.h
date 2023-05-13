#pragma once

#include "config.h"
#include "address.h"

#include <netpp/connector.h>
namespace net
{

//-----------------------------------------------------------------------------
/// Init network services with specified config.
//-----------------------------------------------------------------------------
void init_services(const service_config& config = {});

//-----------------------------------------------------------------------------
/// Deinits the network services and joins all worker threads.
//-----------------------------------------------------------------------------
void deinit_services();

//-------//
/// TCP ///
//-------//
//-----------------------------------------------------------------------------
/// Creates a tcp v4/v6 server
//-----------------------------------------------------------------------------
connector_ptr create_tcp_server(uint16_t port, std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a tcp v4/v6 client with hostname
//-----------------------------------------------------------------------------
connector_ptr create_tcp_client(const std::string& host, uint16_t port,
                                std::chrono::seconds heartbeat = std::chrono::seconds{0}, bool auto_reconnect = true);

//-----------------------------------------------------------------------------
/// Creates a tcp v4/v6 client with ip::address
//-----------------------------------------------------------------------------
connector_ptr create_tcp_client(const ip::address& address, uint16_t port,
                                std::chrono::seconds heartbeat = std::chrono::seconds{0}, bool auto_reconnect = true);

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 server
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_server(uint16_t port, const ssl_config& config = {},
                                    std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 client with hostname
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port, const ssl_config& config = {},
                                    std::chrono::seconds heartbeat = std::chrono::seconds{0}, bool auto_reconnect = true);

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 client with ip::address
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_client(const ip::address& address, uint16_t port, const ssl_config& config = {},
                                    std::chrono::seconds heartbeat = std::chrono::seconds{0}, bool auto_reconnect = true);

//-----------------------------------------------------------------------------
/// Creates a tcp local(domain socket) server.
/// Only available on platforms that support unix domain sockets.
//-----------------------------------------------------------------------------
connector_ptr create_tcp_local_server(const std::string& file);

//-----------------------------------------------------------------------------
/// Creates a tcp local(domain socket) client.
/// Only available on platforms that support unix domain sockets.
//-----------------------------------------------------------------------------
connector_ptr create_tcp_local_client(const std::string& file);

//-----------------------------------------------------------------------------
/// Creates a secure tcp local(domain socket) server.
/// Only available on platforms that support unix domain sockets.
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_local_server(const std::string& file, const ssl_config& config = {});

//-----------------------------------------------------------------------------
/// Creates a secure tcp local(domain socket) client.
/// Only available on platforms that support unix domain sockets.
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_local_client(const std::string& file, const ssl_config& config = {});

//-------//
/// UDP ///
//-------//
//-----------------------------------------------------------------------------
/// Creates a udp unicast server.
/// one - one communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_unicast_server(uint16_t port,
                                        std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a udp unicast client.
/// one - one communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port,
                                        std::chrono::seconds heartbeat = std::chrono::seconds{0});
connector_ptr create_udp_unicast_client(const ip::address& unicast_address, uint16_t port,
                                        std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a udp multicast (connector).
/// one/many senders - one/many recievers communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_multicaster(const std::string& multicast_address, uint16_t port);
connector_ptr create_udp_multicaster(const ip::address& multicast_address, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp broacast connector.
/// one sender - one/many recievers communication via udp.
/// IMPORTANT! -> There are no broadcast addresses in IPv6, their function being
/// superseded by multicast addresses.
/// Consider Using multicast.
//-----------------------------------------------------------------------------
connector_ptr create_udp_broadcaster(const std::string& host_address, const std::string& net_mask, uint16_t port);
connector_ptr create_udp_broadcaster(const ip::address_v4& host_address, const ip::address_v4& net_mask, uint16_t port);

std::string host_name();

//-----------------------------------------------------------------------------
/// Return all interfaces with addreses belong to them
/// return map with mac address as key and vector of addresses
//-----------------------------------------------------------------------------
struct interface
{
    // interface name
    std::string id;
    // mac address
    std::string hw_address;
    // ip's associated with this interface
    std::vector<ip::address> addresses;
};

using interfaces_t = std::vector<interface>;
interfaces_t get_interfaces();

//-----------------------------------------------------------------------------
/// Return all addresses on this machine
/// return vector of ip::address
//-----------------------------------------------------------------------------
std::vector<ip::address> get_addresses();

ip::address resolve_ip_address(const std::string& hostname, std::error_code& ec);

//-----------------------------------------------------------------------------
/// Return address and port by given endpoint
/// return tuple of ip::address and uint16_t
//-----------------------------------------------------------------------------
std::tuple<ip::address, uint16_t> resolve_endpoint(const std::string& endpoint, std::error_code& ec);
} // namespace net
