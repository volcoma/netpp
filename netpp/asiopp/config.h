#pragma once
#include <cstdlib>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace net
{

struct service_config
{
	size_t workers = std::thread::hardware_concurrency();
	std::function<void(std::thread&, const std::string&)> set_thread_name = nullptr;
};

struct ssl_certificate
{
	using properties_t = std::map<std::string, std::string>;

	std::string serial_number;

	std::string issuer;
	properties_t issuer_properties;

	std::string subject;
	properties_t subject_properties;

	std::vector<uint8_t> sha256;
	std::vector<uint8_t> sha512;

	int version{0};
	int public_key_bit_size{0};
};

enum class ssl_method : uint8_t
{
	/// Generic SSL version 2.
	sslv2,

	/// SSL version 2 client.
	sslv2_client,

	/// SSL version 2 server.
	sslv2_server,

	/// Generic SSL version 3.
	sslv3,

	/// SSL version 3 client.
	sslv3_client,

	/// SSL version 3 server.
	sslv3_server,

	/// Generic TLS version 1.
	tlsv1,

	/// TLS version 1 client.
	tlsv1_client,

	/// TLS version 1 server.
	tlsv1_server,

	/// Generic SSL/TLS.
	sslv23,

	/// SSL/TLS client.
	sslv23_client,

	/// SSL/TLS server.
	sslv23_server,

	/// Generic TLS version 1.1.
	tlsv11,

	/// TLS version 1.1 client.
	tlsv11_client,

	/// TLS version 1.1 server.
	tlsv11_server,

	/// Generic TLS version 1.2.
	tlsv12,

	/// TLS version 1.2 client.
	tlsv12_client,

	/// TLS version 1.2 server.
	tlsv12_server,

	/// Generic TLS.
	tls,

	/// TLS client.
	tls_client,

	/// TLS server.
	tls_server
};

enum ssl_handshake : uint8_t
{
	/// Deduced from the socket communication.
	automatic,

	/// Perform handshaking as a client.
	client,

	/// Perform handshaking as a server.
	server
};

struct ssl_config
{
	std::string cert_auth_file;
	std::string cert_chain_file;
	std::string private_key_file;
	std::string dh_file;
	std::string private_key_password;
	std::function<bool(const ssl_certificate&)> verify_callback;
	ssl_method method = ssl_method::tlsv12;
	bool require_peer_cert = false;

	// This can be used to specify a different behaviour
	// e.g a tcp server can be treated as a ssl hanshake client
	// or otherwise a tcp client can be a ssl hanshake server
	// in a one-way authentication
	//
	// Automatic mode will have the standard behavior.
	// Do not change if you don't know what you are doing.
	ssl_handshake handshake_type = ssl_handshake::automatic;
};

} // namespace net
