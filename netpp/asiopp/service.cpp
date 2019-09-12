#include "service.h"

#include "tcp/basic_client.hpp"
#include "tcp/basic_server.hpp"

#include "tcp/basic_ssl_client.hpp"
#include "tcp/basic_ssl_server.hpp"

#include "udp/basic_client.h"
#include "udp/basic_server.h"

#include <asio/ip/host_name.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#ifdef ASIO_HAS_LOCAL_SOCKETS
#include <asio/local/stream_protocol.hpp>
#include <cstdio>
#endif

#include <asio/io_service.hpp>

namespace net
{
using namespace asio;

#define this_func "[net::" << __func__ << "]"

namespace
{
uint32_t init_count = 0;

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

void create_service_threads(const service_config& config)
{
	auto& threads = get_service_threads();

	if(!threads.empty())
	{
		return;
	}

	threads.reserve(config.workers);
	for(size_t i = 0; i < config.workers; ++i)
	{
		threads.emplace_back([]() {
			try
			{
				get_io_context().run();
			}
			catch(std::exception& e)
			{
				log() << this_func << " Exception: " << e.what();
			}
		});
		if(config.set_thread_name)
		{
			config.set_thread_name(threads.back(), "net_service");
		}
	}
}
}

void init_services(const service_config& init)
{
	if(init_count != 0)
	{
		return;
	}
	++init_count;

	get_work();
	create_service_threads(init);

	log() << this_func << " Successful.";
}

void deinit_services()
{
	--init_count;
	if(init_count != 0)
	{
		return;
	}

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
			log() << this_func << " Exception: " << e.what();
		}
	}
	threads.clear();

	log() << this_func << " Successful.";
}

connector_ptr create_tcp_server(uint16_t port, std::chrono::seconds heartbeat)
{
	using type = net::tcp::basic_server<asio::ip::tcp>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(type::protocol::v6(), port);
	try
	{
		return std::make_shared<type>(net_context, endpoint, heartbeat);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
	return nullptr;
}

connector_ptr create_tcp_client(const std::string& host, uint16_t port, std::chrono::seconds heartbeat)
{
	using type = net::tcp::basic_client<asio::ip::tcp>;
	auto& net_context = get_io_context();
	type::protocol::resolver r(net_context);
	error_code ec;
	auto res = r.resolve(type::protocol::resolver::query(host, std::to_string(port)), ec);
	if(ec)
	{
		log() << this_func << " Resolving host failed - " << ec.message();
		return nullptr;
	}

	auto endpoint = res->endpoint();
	try
	{
		return std::make_shared<type>(net_context, endpoint, heartbeat);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
	return nullptr;
}

connector_ptr create_tcp_ssl_server(uint16_t port, const ssl_config& config, std::chrono::seconds heartbeat)
{
	using type = net::tcp::basic_ssl_server<asio::ip::tcp>;

	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(type::protocol::v6(), port);
	try
	{
		return std::make_shared<type>(net_context, endpoint, config, heartbeat);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
	return nullptr;
}

connector_ptr create_tcp_ssl_client(const std::string& host, uint16_t port, const ssl_config& config,
									std::chrono::seconds heartbeat)
{
	using type = net::tcp::basic_ssl_client<asio::ip::tcp>;

	auto& net_context = get_io_context();
	type::protocol::resolver r(net_context);
	error_code ec;
	auto res = r.resolve(type::protocol::resolver::query(host, std::to_string(port)));
	if(ec)
	{
		log() << this_func << " Resolving host failed - " << ec.message();
		return nullptr;
	}
	auto endpoint = res->endpoint();
	try
	{
		return std::make_shared<type>(net_context, endpoint, config, heartbeat);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
	return nullptr;
}

connector_ptr create_tcp_local_server(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_server<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	std::remove(file.c_str());
	type::protocol_endpoint endpoint(file);
	try
	{
		return std::make_shared<type>(net_context, endpoint);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
#else
	(void)file;
	log() << this_func << " Local(domain) sockets are not supported.";
#endif
	return nullptr;
}

connector_ptr create_tcp_local_client(const std::string& file)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_client<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	try
	{
		return std::make_shared<type>(net_context, endpoint);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
#else
	(void)file;
	log() << this_func << " Local(domain) sockets are not supported.";
#endif
	return nullptr;
}

connector_ptr create_tcp_ssl_local_server(const std::string& file, const ssl_config& config)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_ssl_server<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	std::remove(file.c_str());
	type::protocol_endpoint endpoint(file);
	try
	{
		return std::make_shared<type>(net_context, endpoint, config);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
#else
	(void)file;
	(void)config;
	log() << this_func << " Local(domain) sockets are not supported.";
#endif
	return nullptr;
}

connector_ptr create_tcp_ssl_local_client(const std::string& file, const ssl_config& config)
{
#ifdef ASIO_HAS_LOCAL_SOCKETS
	using type = net::tcp::basic_ssl_client<asio::local::stream_protocol>;
	auto& net_context = get_io_context();
	type::protocol_endpoint endpoint(file);
	try
	{
		return std::make_shared<type>(net_context, endpoint, config);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
#else
	(void)file;
	(void)config;
	log() << this_func << " Local(domain) sockets are not supported.";
#endif
	return nullptr;
}


connector_ptr create_udp_unicast_server(uint16_t port, std::chrono::seconds heartbeat)
{
    auto& net_context = get_io_context();

    asio::ip::udp::endpoint endpoint(asio::ip::address_v6::any(), port);
    try
    {
        return std::make_shared<net::udp::basic_server>(net_context, endpoint, heartbeat);
    }
    catch(const std::exception& e)
    {
        log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
    }
    return nullptr;
}

connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port, std::chrono::seconds heartbeat)
{
    auto& net_context = get_io_context();
    error_code ec;
    auto address = asio::ip::address::from_string(unicast_address, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }
    if(address.is_multicast())
    {
        log() << this_func << " Failed. You provided a multicast address.";
        return nullptr;
    }

    asio::ip::udp::endpoint endpoint(address, port);
    try
    {
        return std::make_shared<net::udp::basic_client>(net_context, endpoint, heartbeat);
    }
    catch(const std::exception& e)
    {
        log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
    }
    return nullptr;
}

connector_ptr create_udp_multicaster(const std::string& multicast_address, uint16_t port)
{
	auto& net_context = get_io_context();
	error_code ec;
	auto address = asio::ip::address::from_string(multicast_address, ec);
	if(ec)
	{
		log() << this_func << " Failed : " << ec.message();
		return nullptr;
	}
	if(!address.is_multicast())
	{
		log() << this_func << " Failed. You must specify a valid multicast address.";
		return nullptr;
	}
	asio::ip::udp::endpoint endpoint(address, port);
	try
	{
        return std::make_shared<net::udp::basic_client>(net_context, endpoint);
	}
	catch(const std::exception& e)
	{
		log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
	}
	return nullptr;
}

connector_ptr create_udp_broadcaster(const std::string& host_address, const std::string& net_mask, uint16_t port)
{
    auto& net_context = get_io_context();
    error_code ec {};

    auto host_addr = asio::ip::address_v4::from_string(host_address, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    auto net_mask_addr = asio::ip::address_v4::from_string(net_mask, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    auto address = asio::ip::address_v4::broadcast(host_addr, net_mask_addr);

    asio::ip::udp::endpoint endpoint(address, port);
    try
    {
        return std::make_shared<net::udp::basic_client>(net_context, endpoint);
    }
    catch(const std::exception& e)
    {
        log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
    }
    return nullptr;
}

std::string host_name()
{
	return asio::ip::host_name();
}

std::vector<std::string> host_addresses(uint32_t v_flags, uint32_t t_flags)
{
	std::vector<std::string> result;
	auto host = host_name();

	auto& net_context = get_io_context();
	asio::ip::tcp::resolver r(net_context);
	std::for_each(r.resolve({host, ""}), {}, [&](const auto& re) {
		auto address = re.endpoint().address();

		bool v_add = false;
		v_add |= (v_flags & version_flags::v4) && address.is_v4();
		v_add |= (v_flags & version_flags::v6) && address.is_v6();

		bool t_add = false;
		t_add |= (t_flags & type_flags::unicast) && !address.is_multicast();
		t_add |= (t_flags & type_flags::multicast) && address.is_multicast();

		if(v_add & t_add)
		{
			result.emplace_back(address.to_string());
		}
	});

    return result;
}

} // namespace net
