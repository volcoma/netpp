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
io_context& get_io_context()
{
	static io_context context;
	static io_context::work work(context);
	return context;
}

std::vector<std::thread>& workers()
{
	static std::vector<std::thread> threads = []() {
		std::vector<std::thread> result;
		for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
		{
			std::thread th([]() { get_io_context().run(); });
			result.emplace_back(std::move(th));
		}

		return result;
	}();
	return threads;
}
}

void init_services()
{
	get_io_context();
	workers();
}

void deinit_services()
{
	get_io_context().stop();

	auto& threads = workers();

	for(auto& worker : threads)
	{
		if(worker.joinable())
		{
			worker.join();
		}
	}
	threads.clear();
}
connector_ptr create_tcp_server(uint16_t port)
{
	auto& net_context = get_io_context();
	net::tcp::endpoint listen_endpoint(tcp::v6(), port);
	return std::make_shared<net::tcp_server>(net_context, listen_endpoint);
}

net::connector_ptr create_tcp_client(const std::string& host, const std::string& port)
{
	auto& net_context = get_io_context();
	tcp::resolver r(net_context);
	auto res = r.resolve(host, port);
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<tcp_client>(net_context, endpoint);
}

connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
									const std::string& private_key_file, const std::string& dh_file)
{
	auto& net_context = get_io_context();
	tcp::endpoint listen_endpoint(tcp::v6(), port);
	return std::make_shared<tcp_ssl_server>(net_context, listen_endpoint, cert_chain_file, private_key_file,
											dh_file);
}

connector_ptr create_tcp_ssl_client(const std::string& host, const std::string& port,
									const std::string& cert_file)
{
	auto& net_context = get_io_context();
	tcp::resolver r(net_context);
	auto res = r.resolve(host, port);
	auto endpoint = res.begin()->endpoint();
	return std::make_shared<tcp_ssl_client>(net_context, endpoint, cert_file);
}

net::connector_ptr create_tcp_local_server(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	auto& net_context = get_io_context();
	tcp_local_server::protocol_endpoint endpoint(file);
	return std::make_shared<tcp_local_server>(net_context, endpoint);
#else
	return nullptr;
#endif
}

net::connector_ptr create_tcp_local_client(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	auto& net_context = get_io_context();
	tcp_local_server::protocol_endpoint endpoint(file);
	return std::make_shared<tcp_local_client>(net_context, endpoint);
#else
	return nullptr;
#endif
}

} // namespace net
