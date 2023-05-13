#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace net
{

namespace ip
{

class address_v4
{
public:
    using bytes_type = std::array<uint8_t, 4>;
    using uint_type = uint32_t;

    address_v4() noexcept = default;
    address_v4(uint_type addr) noexcept;
    address_v4(const bytes_type& bytes) noexcept;

    address_v4(const address_v4&) noexcept = default;
    address_v4(address_v4&&) noexcept = default;

    address_v4& operator=(const address_v4& other) noexcept = default;
    address_v4& operator=(address_v4&& other) noexcept = default;

    /// Get the address in bytes, in network byte order.
    bytes_type to_bytes() const noexcept;

    /// Get the address as an unsigned integer in host byte order
    uint_type to_uint() const noexcept;

    /// Get the address as a string in dotted decimal format.
    std::string to_string() const;

    /// Determine whether the address is a loopback address.
    bool is_loopback() const;

    /// Determine whether the address is unspecified.
    bool is_unspecified() const;

    /// Determine whether the address is a multicast address.
    bool is_multicast() const;

    /// Obtain an address object that represents any address.
    static address_v4 any();

    /// Obtain an address object that represents the loopback address.
    static address_v4 loopback();

    /// Obtain an address object that represents the broadcast address.
    static address_v4 broadcast();

    /// Compare addresses for ordering.
    friend bool operator<(const address_v4& a1, const address_v4& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator>(const address_v4& a1, const address_v4& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator<=(const address_v4& a1, const address_v4& a2) noexcept;

    /// Compare addresses for ordering.
    friend bool operator>=(const address_v4& a1, const address_v4& a2) noexcept;
private:
    uint_type raw_data_ {};
};

address_v4 make_address_v4(const address_v4::bytes_type& bytes) noexcept;
address_v4 make_address_v4(address_v4::uint_type uint) noexcept;


}

}
