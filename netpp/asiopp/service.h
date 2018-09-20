#pragma once
#include <netpp/connector.h>

namespace net
{

//-----------------------------------------------------------------------------
/// Init network services with specified workers count.
//-----------------------------------------------------------------------------
void init_services(size_t workers);

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
connector_ptr create_tcp_server(uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a tcp v4/v6 client
//-----------------------------------------------------------------------------
connector_ptr create_tcp_client(const std::string& host, uint16_t port);

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 server
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
   								 const std::string& private_key_file, const std::string& dh_file);

//-----------------------------------------------------------------------------
/// Creates a secure tcp v4/v6 client
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port,
   								 const std::string& cert_file);

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
connector_ptr create_tcp_ssl_local_server(const std::string& file, const std::string& cert_chain_file,
   									   const std::string& private_key_file,
   									   const std::string& dh_file);

//-----------------------------------------------------------------------------
/// Creates a secure tcp local(domain socket) client.
/// Only available on platforms that support unix domain sockets.
//-----------------------------------------------------------------------------
connector_ptr create_tcp_ssl_local_client(const std::string& file, const std::string& cert_file);

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
} // namespace net
