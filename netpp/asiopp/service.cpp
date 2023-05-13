#include "service.h"

#include "tcp/basic_client.hpp"
#include "tcp/basic_server.hpp"

#include "tcp/basic_ssl_client.hpp"
#include "tcp/basic_ssl_server.hpp"

#include "udp/basic_client.h"
#include "udp/basic_server.h"

#include "utils/interfaces.h"

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

namespace
{
asio::ip::address convert(const ip::address& addr)
{
    if (addr.is_v4())
    {
        auto v4 = addr.to_v4();
        return asio::ip::address_v4(v4.to_uint());
    }
    else if (addr.is_v6())
    {
        auto v6 = addr.to_v6();
        return asio::ip::address_v6(v6.to_bytes(), v6.scope_id());
    }

    return {};
}
}

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

connector_ptr create_tcp_client(const std::string& host, uint16_t port, std::chrono::seconds heartbeat, bool auto_reconnect)
{
    error_code ec;
    auto ip = resolve_ip_address(host, ec);
    if(ec)
    {
        log() << this_func << " Resolving host failed - " << ec.message();
        return nullptr;
    }

    return create_tcp_client(ip, port, heartbeat, auto_reconnect);
}

connector_ptr create_tcp_client(const ip::address& address, uint16_t port, std::chrono::seconds heartbeat, bool auto_reconnect)
{
    using type = net::tcp::basic_client<asio::ip::tcp>;
    asio::ip::basic_endpoint<asio::ip::tcp> endpoint(convert(address), port);

    try
    {
        auto& net_context = get_io_context();
        return std::make_shared<type>(net_context, endpoint, heartbeat, auto_reconnect);
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
                                    std::chrono::seconds heartbeat, bool auto_reconnect)
{
    error_code ec;
    auto ip = resolve_ip_address(host, ec);
    if(ec)
    {
        log() << this_func << " Resolving host failed - " << ec.message();
        return nullptr;
    }

    return create_tcp_ssl_client(ip, port, config, heartbeat, auto_reconnect);
}

connector_ptr create_tcp_ssl_client(const ip::address& address, uint16_t port, const ssl_config& config,
                                    std::chrono::seconds heartbeat, bool auto_reconnect)
{
    using type = net::tcp::basic_ssl_client<asio::ip::tcp>;
    asio::ip::basic_endpoint<asio::ip::tcp> endpoint(convert(address), port);

    try
    {
        auto& net_context = get_io_context();
        return std::make_shared<type>(net_context, endpoint, config, heartbeat, auto_reconnect);
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

connector_ptr create_udp_unicast_client(const std::string& unicast_address, uint16_t port,
                                        std::chrono::seconds heartbeat)
{
    error_code ec;
    auto address = resolve_ip_address(unicast_address, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    return create_udp_unicast_client(address, port, heartbeat);
}

connector_ptr create_udp_unicast_client(const ip::address& unicast_address, uint16_t port,
                                        std::chrono::seconds heartbeat)
{

    auto address = convert(unicast_address);
    if(address.is_multicast())
    {
        log() << this_func << " Failed. You provided a multicast address.";
        return nullptr;
    }

    asio::ip::udp::endpoint endpoint(address, port);
    try
    {
        auto& net_context = get_io_context();
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
    error_code ec;
    auto address = resolve_ip_address(multicast_address, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    return create_udp_multicaster(address, port);
}

connector_ptr create_udp_multicaster(const ip::address& multicast_address, uint16_t port)
{
    auto address = convert(multicast_address);
    if(!address.is_multicast())
    {
        log() << this_func << " Failed. You must specify a valid multicast address.";
        return nullptr;
    }
    asio::ip::udp::endpoint endpoint(address, port);
    try
    {
        auto& net_context = get_io_context();
        return std::make_shared<net::udp::basic_client>(net_context, endpoint);
    }
    catch(const std::exception& e)
    {
        log() << this_func << " Failed for endpoint - " << endpoint << " : " << e.what();
    }
    return nullptr;
}

connector_ptr create_udp_broadcaster(const std::string& host_address, const std::string& net_mask,
                                     uint16_t port)
{
    error_code ec{};
    auto host_addr = resolve_ip_address(host_address, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    if (!host_addr.is_v4())
    {
        log() << this_func << " Failed : Host address is not IPV4. Host address: " << host_address;
        return nullptr;
    }

    auto net_mask_addr = resolve_ip_address(net_mask, ec);
    if(ec)
    {
        log() << this_func << " Failed : " << ec.message();
        return nullptr;
    }

    if (!net_mask_addr.is_v4())
    {
        log() << this_func << " Failed : Netmask is not IPV4. Netmask address: " << net_mask;
        return nullptr;
    }

    return create_udp_broadcaster(host_addr.to_v4(), net_mask_addr.to_v4(), port);
}

connector_ptr create_udp_broadcaster(const ip::address_v4& host_address, const ip::address_v4& net_mask, uint16_t port)
{
    auto host_addr = asio::ip::make_address_v4(host_address.to_uint());
    auto net_mask_addr = asio::ip::make_address_v4(net_mask.to_uint());
    auto address = asio::ip::address_v4::broadcast(host_addr, net_mask_addr);

    asio::ip::udp::endpoint endpoint(address, port);
    try
    {
        auto& net_context = get_io_context();
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

interfaces_t get_interfaces()
{
    auto interfaces = utils::interfaces::get_local_interfaces();
    interfaces_t result {};

    result.reserve(interfaces.size());
    for (auto& kvp : interfaces)
    {
        const auto& id = kvp.first;
        auto& data = kvp.second;
        auto& addresses = data.addresses;

        result.emplace_back();
        auto& iface = result.back();
        iface.id = id;
        iface.hw_address = data.mac;
        iface.addresses = std::move(addresses);
    }

    return result;
}


std::vector<ip::address> get_addresses()
{
    auto interfaces = utils::interfaces::get_local_interfaces();
    std::vector<ip::address> result {};
    for (auto& kvp : interfaces)
    {
        auto& addresses = kvp.second.addresses;
        std::move(std::begin(addresses), std::end(addresses), std::back_inserter(result));
    }

    return result;
}

ip::address resolve_ip_address(const std::string& hostname, std::error_code& ec)
{
    auto& net_context = get_io_context();
    asio::ip::tcp::resolver r(net_context);
    auto it = r.resolve(asio::ip::tcp::resolver::query(hostname, ""), ec);
    if (ec)
    {
        return {};
    }

    auto endpoint = it->endpoint();
    auto address = endpoint.address();

    if (address.is_v4())
    {
        auto v4 = address.to_v4();
        return ip::make_address_v4(v4.to_uint());
    }
    else if (address.is_v6())
    {
        auto v6 = address.to_v6();
        return ip::make_address_v6(v6.to_bytes(), v6.scope_id());
    }

    return {};
}

std::tuple<ip::address, uint16_t> resolve_endpoint(const std::string &endpoint, std::error_code &ec)
{
    auto pos = endpoint.find_last_of(':');
    if (pos == std::string::npos)
    {
        return {};
    }

    auto address_str = endpoint.substr(0, pos);

    //remove [ ] from address' string if it's IPV6
    {
        auto start_pos = address_str.find_first_of('[');
        if (start_pos != std::string::npos)
        {
            auto end_pos = address_str.find_last_of(']');
            if (end_pos != std::string::npos)
            {
                address_str.erase(end_pos, 1);
                address_str.erase(start_pos, 1);
            }
        }
    }

    auto address = asio::ip::address::from_string(address_str, ec);
    if (ec)
    {
        return {};
    }

    auto port_str = endpoint.substr(pos + 1);
    uint16_t port = std::stoi(port_str);

    if (address.is_v4())
    {
        auto v4 = address.to_v4();
        return {ip::make_address_v4(v4.to_uint()), port};
    }
    else if (address.is_v6())
    {
        auto v6 = address.to_v6();
        return {ip::make_address_v6(v6.to_bytes(), v6.scope_id()), port};
    }

    return {};
}

} // namespace net
