#include "address.h"

namespace net
{
namespace ip
{

address::address(const address_v4& ipv4_address) noexcept
    : type_(ipv4)
    , ipv4_address_(ipv4_address)
{
}

address::address(const address_v6& ipv6_address) noexcept
    : type_(ipv6)
    , ipv6_address_(ipv6_address)
{
}

bool address::is_v4() const
{
    return type_ == ipv4;
}

bool address::is_v6() const
{
    return type_ == ipv6;
}

address_v4 address::to_v4() const noexcept
{
    return ipv4_address_;
}

address_v6 address::to_v6() const noexcept
{
    return ipv6_address_;
}

std::string address::to_string() const
{
    if (is_v4())
    {
        return to_v4().to_string();
    }

    return to_v6().to_string();
}

bool address::is_loopback() const
{
    if (is_v4())
    {
        return to_v4().is_loopback();
    }

    return to_v6().is_loopback();
}

bool address::is_unspecified() const
{
    if (is_v4())
    {
        return to_v4().is_unspecified();
    }

    return to_v6().is_unspecified();
}

bool address::is_multicast() const
{
    if (is_v4())
    {
        return to_v4().is_multicast();
    }

    return to_v6().is_multicast();
}

bool operator==(const address& a1, const address& a2) noexcept
{
    if (a1.is_v4() && a2.is_v4())
    {
        return a1.to_v4() == a2.to_v4();
    }
    else if (a1.is_v6() && a2.is_v6())
    {
        return a1.to_v6() == a2.to_v6();
    }

    return false;
}

bool operator!=(const address& a1, const address& a2) noexcept
{
    return !(a1 == a2);
}

bool operator<(const address& a1, const address& a2) noexcept
{
    if (a1.is_v4() && a2.is_v4())
    {
        return a1.to_v4() < a2.to_v4();
    }
    else if (a1.is_v6() && a2.is_v6())
    {
        return a1.to_v6() < a2.to_v6();
    }
    else if (a1.is_v4() && a2.is_v6())
    {
        return true;
    }

    return false;
}

bool operator>(const address& a1, const address& a2) noexcept
{
    return a2 < a1;
}

bool operator<=(const address& a1, const address& a2) noexcept
{
    return !(a2 < a1);
}

bool operator>=(const net::ip::address& a1, const address& a2) noexcept
{
    return !(a1 < a2);
}



}
}
