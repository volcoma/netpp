#include "service.h"

#include "tcp/tcp_client.h"
#include "tcp/tcp_server.h"

#include "tcp/tcp_ssl_client.h"
#include "tcp/tcp_ssl_server.h"

#include "tcp/tcp_local_client.h"
#include "tcp/tcp_local_server.h"

#include <asio/io_context.hpp>

namespace net
{
using namespace asio;

namespace
{
auto& get_io_context()
{
	static io_context context;
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
		threads.emplace_back([]() { get_io_context().run(); });
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
	using connector_type = net::tcp_server;

	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(tcp::v6(), port);
	return std::make_shared<connector_type>(net_context, endpoint);
}

net::connector_ptr create_tcp_client(const std::string& host, const std::string& port)
{
	using connector_type = net::tcp_server;

	auto& net_context = get_io_context();
	connector_type::protocol::resolver r(net_context);
	auto res = r.resolve(host, port);
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<connector_type>(net_context, endpoint);
}

connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
									const std::string& private_key_file, const std::string& dh_file)
{
	using connector_type = net::tcp_ssl_server;

	auto& net_context = get_io_context();
	connector_type::protocol_endpoint endpoint(tcp::v6(), port);
	return std::make_shared<connector_type>(net_context, endpoint, cert_chain_file, private_key_file,
											dh_file);
}

connector_ptr create_tcp_ssl_client(const std::string& host, const std::string& port,
									const std::string& cert_file)
{
	using connector_type = net::tcp_ssl_client;

	auto& net_context = get_io_context();
	connector_type::protocol::resolver r(net_context);
	auto res = r.resolve(host, port);
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<connector_type>(net_context, endpoint, cert_file);
}

net::connector_ptr create_tcp_local_server(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using connector_type = net::tcp_local_server;
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
	using connector_type = net::tcp_local_client;
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
	using connector_type = net::tcp_ssl_local_server;
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
	using connector_type = net::tcp_ssl_local_client;
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
