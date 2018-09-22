#include "service.h"

#include "tcp/basic_client.hpp"
#include "tcp/basic_server.hpp"

#include "tcp/basic_ssl_client.hpp"
#include "tcp/basic_ssl_server.hpp"

#include "udp/basic_reciever.h"
#include "udp/basic_sender.h"

#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
#endif

#include <asio/io_service.hpp>

namespace net
{
using namespace asio;

namespace
{
auto& get_io_context()
{
	static asio::io_service context;
	return context;
}

auto& get_work()
{
	static auto work = std::make_shared<asio::io_service::work>(get_io_context());
	return work;
}

auto& get_service_threads()
{
	static std::vector<std::thread> threads;
	return threads;
}

void create_service_threads(size_t workers = 1)
{
	auto& threads = get_service_threads();
	threads.reserve(workers);
	for(size_t i = 0; i < workers; ++i)
	{
		threads.emplace_back([]() {
			try
			{
				get_io_context().run();
			}
			catch(std::exception& e)
			{
				log() << "Exception: " << e.what();
			}
		});
	}
}
}

void init_services(size_t workers)
{
	log() << "init network services";

	get_work();
	create_service_threads(workers);
}

void deinit_services()
{
	log() << "deinit network services";
	get_work().reset();
	get_io_context().stop();

	auto& threads = get_service_threads();

	for(auto& thread : threads)
	{
		try
		{
			if(thread.joinable())
			{
				thread.join();
			}
		}
		catch(const std::exception& e)
		{
			log() << "Exception: " << e.what();
		}
	}
	threads.clear();
}

connector_ptr create_tcp_server(uint16_t port)
{
	using type = net::tcp::basic_server<asio::ip::tcp>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(type::protocol::v6(), port);
	return std::make_shared<type>(net_context, endpoint);
}

connector_ptr create_tcp_client(const std::string& host, uint16_t port)
{
	using type = net::tcp::basic_client<asio::ip::tcp>;
	auto& net_context = get_io_context();
	type::protocol::resolver r(net_context);
	error_code ec;
	auto res = r.resolve(type::protocol::resolver::query(host, std::to_string(port)), ec);
	if(ec)
	{
		log() << "Resoving host failed : " << ec.message();
		return nullptr;
	}

	auto endpoint = res->endpoint();
	return std::make_shared<type>(net_context, endpoint);
}

connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
									const std::string& private_key_file, const std::string& dh_file,
                                    const std::string& private_key_password)
{
	using type = net::tcp::basic_ssl_server<asio::ip::tcp>;

	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(type::protocol::v6(), port);
	return std::make_shared<type>(net_context, endpoint, cert_chain_file, private_key_file, dh_file, private_key_password);
}

connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port, const std::string& cert_file)
{
	using type = net::tcp::basic_ssl_client<asio::ip::tcp>;

	auto& net_context = get_io_context();
	type::protocol::resolver r(net_context);
	error_code ec;
	auto res = r.resolve(type::protocol::resolver::query(host, std::to_string(port)));
	if(ec)
	{
		log() << "Resoving host failed : " << ec.message();
		return nullptr;
	}
	auto endpoint = res->endpoint();
	return std::make_shared<type>(net_context, endpoint, cert_file);
}

connector_ptr create_tcp_local_server(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_server<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	return std::make_shared<type>(net_context, endpoint);
#else
	(void)file;
	log() << "Local(domain) sockets are not supported.";
	return nullptr;
#endif
}

connector_ptr create_tcp_local_client(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_client<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	return std::make_shared<type>(net_context, endpoint);
#else
	(void)file;
	log() << "Local(domain) sockets are not supported.";
	return nullptr;
#endif
}

connector_ptr create_tcp_ssl_local_server(const std::string& file, const std::string& cert_chain_file,
										  const std::string& private_key_file, const std::string& dh_file,
                                          const std::string& private_key_password)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_ssl_server<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	return std::make_shared<type>(net_context, endpoint, cert_chain_file, private_key_file, dh_file, private_key_password);
#else
	(void)file;
	(void)cert_chain_file;
	(void)private_key_file;
	(void)dh_file;
	log() << "Local(domain) sockets are not supported.";
	return nullptr;
#endif
}

connector_ptr create_tcp_ssl_local_client(const std::string& file, const std::string& cert_file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_ssl_client<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	return std::make_shared<type>(net_context, endpoint, cert_file);
#else
	(void)file;
	(void)cert_file;
	log() << "Local(domain) sockets are not supported.";
	return nullptr;
#endif
}

connector_ptr create_udp_unicast_server(const std::string& unicast_address, uint16_t port)
{
	auto& net_context = get_io_context();
	error_code ec;
	auto address = asio::ip::address::from_string(unicast_address, ec);
	if(ec)
	{
		log() << "Creating an udp client failed : " << ec.message();
		return nullptr;
	}
	if(address.is_multicast())
	{
		log() << "Creating a udp unicast server, you provided a multicast address.";
		return nullptr;
	}

	asio::ip::udp::endpoint endpoint(address, port);
	return std::make_shared<net::udp::basic_sender>(net_context, endpoint);
}

connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port)
{
	auto& net_context = get_io_context();

	asio::ip::udp::endpoint listen_endpoint(asio::ip::address_v6::any(), port);
	error_code ec;
	auto address = asio::ip::address::from_string(unicast_address, ec);
	if(ec)
	{
		log() << "Creating an udp client failed : " << ec.message();
		return nullptr;
	}

	if(address.is_multicast())
	{
		log() << "Creating a udp client server, you provided a multicast address.";
		return nullptr;
	}

	asio::ip::udp::endpoint multicast_endpoint(address, port);
	return std::make_shared<net::udp::basic_reciever>(net_context, multicast_endpoint);
}

connector_ptr create_udp_multicast_server(const std::string& multicast_address, uint16_t port)
{
	auto& net_context = get_io_context();
	error_code ec;
	auto address = asio::ip::address::from_string(multicast_address, ec);
	if(ec)
	{
		log() << "Creating an udp multicast server failed : " << ec.message();
		return nullptr;
	}
	if(!address.is_multicast())
	{
		log() << "Must specify a valid multicast address.";
		return nullptr;
	}
	asio::ip::udp::endpoint endpoint(address, port);
	return std::make_shared<net::udp::basic_sender>(net_context, endpoint);
}

connector_ptr create_udp_multicast_client(const std::string& multicast_address, uint16_t port)
{
	auto& net_context = get_io_context();

	asio::ip::udp::endpoint listen_endpoint(asio::ip::address_v6::any(), port);
	error_code ec;
	auto address = asio::ip::address::from_string(multicast_address, ec);
	if(ec)
	{
		log() << "Creating an udp multicast client failed : " << ec.message();
		return nullptr;
	}
	if(!address.is_multicast())
	{
		log() << "Must specify a valid multicast address.";
		return nullptr;
	}

	asio::ip::udp::endpoint multicast_endpoint(address, port);
	return std::make_shared<net::udp::basic_reciever>(net_context, multicast_endpoint);
}

connector_ptr create_udp_broadcast_server(uint16_t port)
{
	asio::ip::address_v4::broadcast();
	auto& net_context = get_io_context();
	auto address = asio::ip::address_v4::broadcast();
	asio::ip::udp::endpoint endpoint(address, port);
	return std::make_shared<net::udp::basic_sender>(net_context, endpoint);
}

connector_ptr create_udp_broadcast_client(uint16_t port)
{
	auto& net_context = get_io_context();
	asio::ip::udp::endpoint listen_endpoint(asio::ip::address_v6::any(), port);
	return std::make_shared<net::udp::basic_reciever>(net_context, listen_endpoint);
}

} // namespace net
