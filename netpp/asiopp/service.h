#pragma once
#include <netpp/connector.h>
#include "config.h"
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
/// Creates a tcp v4/v6 client
//-----------------------------------------------------------------------------
connector_ptr create_tcp_client(const std::string& host, uint16_t port,
								std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 server
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_server(uint16_t port, const ssl_config& config = {},
									std::chrono::seconds heartbeat = std::chrono::seconds{0});

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 client
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port, const ssl_config& config = {},
									std::chrono::seconds heartbeat = std::chrono::seconds{0});

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
/// Creates a udp unicast server(sender).
/// one - one communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_unicast_server(const std::string& unicast_address, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp unicast client(reciever).
/// one - one communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp multicast server(sender).
/// one/many senders - one/many recievers communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_multicast_server(const std::string& multicast_address, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp multicast client(reciever).
/// one/many senders - one/many recievers communication via udp.
//-----------------------------------------------------------------------------
connector_ptr create_udp_multicast_client(const std::string& multicast_address, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp broacast client(reciever).
/// one sender - one/many recievers communication via udp.
/// IMPORTANT! -> There are no broadcast addresses in IPv6, their function being
/// superseded by multicast addresses.
/// Consider Using multicast.
//-----------------------------------------------------------------------------
connector_ptr create_udp_broadcast_server(uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a udp broacast client(reciever).
/// one sender - one/many recievers communication via udp.
/// Consider Using multicast.
//-----------------------------------------------------------------------------
connector_ptr create_udp_broadcast_client(uint16_t port);

namespace version_flags
{
enum
{
	none = 0,
	v4 = 1 << 1,
	v6 = 1 << 2,

	all = v4 | v6
};
}

namespace type_flags
{
enum
{
	none = 0,
	unicast = 1 << 1,
	multicast = 1 << 2,

	all = unicast | multicast
};
}
std::string host_name();
std::vector<std::string> host_addresses(uint32_t v_flags = version_flags::all,
										uint32_t t_flags = type_flags::all);

} // namespace net
