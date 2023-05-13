#pragma once

#include "address_v4.h"
#include "address_v6.h"

namespace net
{
namespace ip
{

class address
{
public:
    address() noexcept = default;
    /// Construct an address from an IPv4 address.
    address(const ip::address_v4& ipv4_address) noexcept;

    /// Construct an address from an IPv6 address.
    address(const ip::address_v6& ipv6_address) noexcept;

    /// Get whether the address is an IP version 4 address.
    bool is_v4() const;

    /// Get whether the address is an IP version 6 address.
    bool is_v6() const;

    /// Get the address as an IP version 4 address.
    ip::address_v4 to_v4() const noexcept;

    /// Get the address as an IP version 6 address.
    ip::address_v6 to_v6() const noexcept;

    /// Get the address as a string.
    std::string to_string() const;

    /// Determine whether the address is a loopback address.
    bool is_loopback() const;

    /// Determine whether the address is unspecified.
    bool is_unspecified() const;

    /// Determine whether the address is a multicast address.
    bool is_multicast() const;

    /// Compare two addresses for equality.
    friend bool operator==(const address& a1, const address& a2) noexcept;

    /// Compare two addresses for inequality.
    friend bool operator!=(const address& a1, const address& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator<(const address& a1, const address& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator>(const address& a1, const address& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator<=(const address& a1, const address& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator>=(const address& a1, const address& a2) noexcept;

private:
    enum {ipv4, ipv6} type_ {};
    address_v4 ipv4_address_ {};
    address_v6 ipv6_address_ {};
};
}
}
