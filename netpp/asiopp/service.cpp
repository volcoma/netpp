#include "service.h"

#include "tcp/client.h"
#include "tcp/server.h"

#include <asio/io_service.hpp>
#include <iostream>
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
	static auto work = asio::make_work_guard(get_io_context());
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
	for(size_t i = 0; i < workers; ++i)
	{
		threads.emplace_back([]() {
			try
			{
				get_io_context().run();
			}
			catch(std::exception& e)
			{
				std::cerr << "Exception: " << e.what() << "\n";
			}
		});
	}
}
}

void init_services(size_t workers)
{
	get_work();
	create_service_threads(workers);
}

void deinit_services()
{
	get_work().reset();
	get_io_context().stop();

	auto& threads = get_service_threads();

	for(auto& thread : threads)
	{
		if(thread.joinable())
		{
			thread.join();
		}
	}
	threads.clear();
}
connector_ptr create_tcp_server(uint16_t port)
{
	using connector_type = net::tcp::server;

	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(asio::ip::tcp::v6(), port);
	return std::make_shared<connector_type>(net_context, endpoint);
}

net::connector_ptr create_tcp_client(const std::string& host, uint16_t port)
{
	using connector_type = net::tcp::client;

	auto& net_context = get_io_context();
	connector_type::protocol::resolver r(net_context);
	auto res = r.resolve(host, std::to_string(port));
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<connector_type>(net_context, endpoint);
}

connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
									const std::string& private_key_file, const std::string& dh_file)
{
	using connector_type = net::tcp::ssl_server;

	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(asio::ip::tcp::v6(), port);
	return std::make_shared<connector_type>(net_context, endpoint, cert_chain_file, private_key_file,
											dh_file);
}

connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port, const std::string& cert_file)
{
	using connector_type = net::tcp::ssl_client;

	auto& net_context = get_io_context();
	connector_type::protocol::resolver r(net_context);
	auto res = r.resolve(host, std::to_string(port));
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<connector_type>(net_context, endpoint, cert_file);
}

net::connector_ptr create_tcp_local_server(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using connector_type = net::tcp::local_server;
	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(file);
	return std::make_shared<connector_type>(net_context, endpoint);
#else
	(void)file;
	return nullptr;
#endif
}

net::connector_ptr create_tcp_local_client(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using connector_type = net::tcp::local_client;
	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(file);
	return std::make_shared<connector_type>(net_context, endpoint);
#else
	(void)file;
	return nullptr;
#endif
}

connector_ptr create_tcp_ssl_local_server(const std::string& file, const std::string& cert_chain_file,
										  const std::string& private_key_file, const std::string& dh_file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using connector_type = net::tcp::ssl_local_server;
	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(file);
	return std::make_shared<connector_type>(net_context, endpoint, cert_chain_file, private_key_file,
											dh_file);
#else
	(void)file;
	(void)cert_chain_file;
	(void)private_key_file;
	(void)dh_file;
	return nullptr;
#endif
}

connector_ptr create_tcp_ssl_local_client(const std::string& file, const std::string& cert_file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using connector_type = net::tcp::ssl_local_client;
	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(file);
	return std::make_shared<connector_type>(net_context, endpoint, cert_file);
#else
	(void)file;
	(void)cert_file;
	return nullptr;
#endif
}

} // namespace net
