#include "interfaces.h"

#include <sstream>
#include <iomanip>
#include <cstring>

#if defined(_WIN32)

#   include <winsock2.h>
// Headers that need to be included after winsock2.h:
#   include <iphlpapi.h>
#   include <ws2ipdef.h>

#elif defined(__APPLE__) || defined(__linux__)
#if defined(__linux__)
#   include <sys/ioctl.h>
#   include <unistd.h>
#endif
#   include <arpa/inet.h>
#   include <ifaddrs.h>
#   include <net/if.h>

#endif

namespace net
{
namespace utils
{
namespace interfaces
{
namespace
{
std::string print_mac(const uint8_t* mac)
{
    std::stringstream ss;

    ss << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[0]) & 0xFF) << ":"
       << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[1]) & 0xFF) << ":"
       << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[2]) & 0xFF) << ":"
       << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[3]) & 0xFF) << ":"
       << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[4]) & 0xFF) << ":"
       << std::hex << std::setw(2) << std::setfill('0') << (uint32_t(mac[5]) & 0xFF);

    return ss.str();
}
}

#if defined(_WIN32)

std::map<std::string, iface_data> get_local_interfaces()
{
    constexpr size_t addresses_size {15000}; // 15K

    std::vector<uint8_t> buffer(addresses_size);
    PIP_ADAPTER_ADDRESSES ifaddrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    DWORD buff_len {addresses_size};

    auto err = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER,
                                    nullptr, ifaddrs, &buff_len);

    if (err != NO_ERROR)
    {
        return {};
    }

    std::map<std::string, iface_data> result {};

    for (auto addr = ifaddrs; addr != nullptr; addr = addr->Next)
    {
        if (addr->OperStatus != IfOperStatusUp)
        {
            continue;
        }

        auto& data = result[addr->AdapterName];

        if (addr->Ipv4Enabled)
        {
            for (auto uaddr = addr->FirstUnicastAddress; uaddr != nullptr; uaddr = uaddr->Next)
            {
                if (uaddr->Address.lpSockaddr->sa_family != AF_INET)
                {
                    continue;
                }

                auto address = reinterpret_cast<const sockaddr_in*>(uaddr->Address.lpSockaddr);
                data.addresses.emplace_back(ip::make_address_v4(ntohl(address->sin_addr.S_un.S_addr)));
            }
        }

        if (addr->Ipv6Enabled)
        {
            for (auto uaddr = addr->FirstUnicastAddress; uaddr != nullptr; uaddr = uaddr->Next)
            {
                if (uaddr->Address.lpSockaddr->sa_family != AF_INET6)
                {
                    continue;
                }

                auto address = reinterpret_cast<const sockaddr_in6*>(uaddr->Address.lpSockaddr);
                ip::address_v6::bytes_type buf;
                std::memcpy(buf.data(), address->sin6_addr.s6_addr, sizeof(address->sin6_addr));
                data.addresses.emplace_back(ip::make_address_v6(buf, address->sin6_scope_id));
            }
        }

        if (addr->PhysicalAddressLength > 0)
        {
            auto mac = addr->PhysicalAddress;
            data.mac = print_mac(reinterpret_cast<const uint8_t*>(mac));
        }
    }

    return result;
}

#elif defined(__APPLE__) || defined(__linux__)

#if defined (__linux__)
std::string get_mac_address(const std::string& interface)
{
    auto fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        return {};
    }

    ifreq ifr {};
    strcpy(ifr.ifr_name, interface.c_str());

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
    {
        close(fd);
        return {};
    }

    close(fd);

    auto mac = ifr.ifr_hwaddr.sa_data;

    return print_mac(reinterpret_cast<const uint8_t*>(mac));
}
#else
std::string get_mac_address(const std::string& interface)
{
    ifaddrs *ifs {};
    if (getifaddrs(&ifs))
    {
        return {};
    }

    for (auto addr = ifs; addr != nullptr; addr = addr->ifa_next)
    {
        // No address? Skip.
        if (addr->ifa_addr == nullptr)
        {
            continue;
        }

        if (interface != addr->ifa_name)
        {
            continue;
        }

        if (addr->ifa_addr->sa_family == AF_LINK)
        {
            auto dl_sock = reinterpret_cast<struct sockaddr_dl*>(addr->ifa_addr);
            auto mac = reinterpret_cast<const uint8_t*>(dl_sock);
            return print_mac(mac);
        }
    }

    return {};
}
#endif

std::map<std::string, iface_data> get_local_interfaces()
{
    ifaddrs *ifs {};
    if (getifaddrs(&ifs))
    {
        return {};
    }

    std::map<std::string, iface_data> result {};

    for (auto addr = ifs; addr != nullptr; addr = addr->ifa_next)
    {
        // No address? Skip.
        if (addr->ifa_addr == nullptr)
        {
            continue;
        }

        if (!(addr->ifa_flags & IFF_UP))
        {
            continue;
        }

        if(addr->ifa_addr->sa_family == AF_INET)
        {
            auto address = reinterpret_cast<const sockaddr_in *>(addr->ifa_addr);
            if (!address)
            {
                continue;
            }

            auto& data = result[addr->ifa_name];
            auto buff = ntohl(address->sin_addr.s_addr);
            data.addresses.emplace_back(ip::make_address_v4(buff));
        }
        else if(addr->ifa_addr->sa_family == AF_INET6)
        {
            auto address = reinterpret_cast<const sockaddr_in6 *>(addr->ifa_addr);
            if (!address)
            {
                continue;
            }

            auto& data = result[addr->ifa_name];
            ip::address_v6::bytes_type buf;
            std::memcpy(buf.data(), address->sin6_addr.s6_addr, sizeof(address->sin6_addr));
            data.addresses.emplace_back(ip::make_address_v6(buf, address->sin6_scope_id));
        }
    }

    freeifaddrs(ifs);

    for (auto& data : result)
    {
        data.second.mac = get_mac_address(data.first);
    }

    return result;
}

#else

std::map<std::string, iface_data> get_local_interfaces()
{
    return {};
}

#endif
}
}
}
