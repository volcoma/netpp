#include "address_v4.h"

#include <asio/ip/address_v4.hpp>

namespace net
{
namespace ip
{
namespace
{
address_v4::uint_type convert_to(const address_v4::bytes_type& bytes) noexcept
{
    address_v4::uint_type result {};

    result |= bytes[0] << 24;
    result |= bytes[1] << 16;
    result |= bytes[2] << 8;
    result |= bytes[3];

    return result;
}

address_v4::bytes_type convert_to(address_v4::uint_type uint) noexcept
{
    address_v4::bytes_type result {};

    result[0] = (uint >> 24) & 0xFF;
    result[1] = (uint >> 16) & 0xFF;
    result[2] = (uint >> 8) & 0xFF;
    result[3] = uint & 0xFF;

    return result;
}
}


address_v4::address_v4(address_v4::uint_type addr) noexcept
    : raw_data_(addr)
{
}

address_v4::address_v4(const address_v4::bytes_type& bytes) noexcept
    : raw_data_(convert_to(bytes))
{
}

address_v4::bytes_type address_v4::to_bytes() const noexcept
{
    return convert_to(raw_data_);
}

address_v4::uint_type address_v4::to_uint() const noexcept
{
    return raw_data_;
}

std::string address_v4::to_string() const
{
    asio::ip::address_v4 addr(raw_data_);
    asio::error_code ec {};

    return addr.to_string(ec);
}

bool address_v4::is_loopback() const
{
    return (to_uint() & 0xFF000000) == 0x7F000000;
}

bool address_v4::is_unspecified() const
{
    return to_uint() == 0;
}

bool address_v4::is_multicast() const
{
    return (to_uint() & 0xF0000000) == 0xE0000000;
}

address_v4 address_v4::any()
{
    return address_v4();
}

address_v4 address_v4::loopback()
{
    return address_v4{0x7F000001};
}

address_v4 address_v4::broadcast()
{
    return address_v4{0xFFFFFFFF};
}

bool operator<(const net::ip::address_v4& a1, const net::ip::address_v4& a2) noexcept
{
    return a1.to_uint() < a2.to_uint();
}

bool operator>(const net::ip::address_v4& a1, const net::ip::address_v4& a2) noexcept
{
    return a1.to_uint() > a2.to_uint();
}

bool operator<=(const address_v4& a1, const address_v4& a2) noexcept
{
    return a1.to_uint() <= a2.to_uint();
}

bool operator>=(const address_v4& a1, const address_v4& a2) noexcept
{
    return a1.to_uint() >= a2.to_uint();
}

address_v4 make_address_v4(const address_v4::bytes_type& bytes) noexcept
{
    return address_v4{bytes};
}

address_v4 make_address_v4(address_v4::uint_type uint) noexcept
{
    return address_v4{uint};
}

}
}
