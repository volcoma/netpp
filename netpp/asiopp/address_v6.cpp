#include "address_v6.h"

#include <asio/ip/address_v6.hpp>

namespace net
{
namespace ip
{

address_v6::address_v6(const address_v6::bytes_type& bytes, unsigned long scope_id) noexcept
    : raw_data_(bytes)
    , scope_id_(scope_id)
{
}

address_v6::bytes_type address_v6::to_bytes() const noexcept
{
    return raw_data_;
}

std::string address_v6::to_string() const
{
    asio::ip::address_v6 tmp(raw_data_, scope_id_);
    asio::error_code ec {};

    return tmp.to_string(ec);
}

bool address_v6::is_loopback() const noexcept
{
    return ((raw_data_[0] == 0) && (raw_data_[1] == 0)
             && (raw_data_[2] == 0) && (raw_data_[3] == 0)
             && (raw_data_[4] == 0) && (raw_data_[5] == 0)
             && (raw_data_[6] == 0) && (raw_data_[7] == 0)
             && (raw_data_[8] == 0) && (raw_data_[9] == 0)
             && (raw_data_[10] == 0) && (raw_data_[11] == 0)
             && (raw_data_[12] == 0) && (raw_data_[13] == 0)
             && (raw_data_[14] == 0) && (raw_data_[15] == 1));
}

bool address_v6::is_unspecified() const noexcept
{
    return ((raw_data_[0] == 0) && (raw_data_[1] == 0)
             && (raw_data_[2] == 0) && (raw_data_[3] == 0)
             && (raw_data_[4] == 0) && (raw_data_[5] == 0)
             && (raw_data_[6] == 0) && (raw_data_[7] == 0)
             && (raw_data_[8] == 0) && (raw_data_[9] == 0)
             && (raw_data_[10] == 0) && (raw_data_[11] == 0)
             && (raw_data_[12] == 0) && (raw_data_[13] == 0)
             && (raw_data_[14] == 0) && (raw_data_[15] == 0));
}

bool address_v6::is_link_local() const noexcept
{
    return ((raw_data_[0] == 0xfe) && ((raw_data_[1] & 0xc0) == 0x80));
}

bool address_v6::is_site_local() const noexcept
{
    return ((raw_data_[0] == 0xfe) && ((raw_data_[1] & 0xc0) == 0xc0));
}

bool address_v6::is_v4_mapped() const noexcept
{
    return ((raw_data_[0] == 0) && (raw_data_[1] == 0)
             && (raw_data_[2] == 0) && (raw_data_[3] == 0)
             && (raw_data_[4] == 0) && (raw_data_[5] == 0)
             && (raw_data_[6] == 0) && (raw_data_[7] == 0)
             && (raw_data_[8] == 0) && (raw_data_[9] == 0)
             && (raw_data_[10] == 0xff) && (raw_data_[11] == 0xff));
}

bool address_v6::is_multicast() const noexcept
{
    return (raw_data_[0] == 0xff);
}

bool address_v6::is_multicast_global() const noexcept
{
    return ((raw_data_[0] == 0xff) && ((raw_data_[1] & 0x0f) == 0x0e));
}

bool address_v6::is_multicast_link_local() const noexcept
{
    return ((raw_data_[0] == 0xff) && ((raw_data_[1] & 0x0f) == 0x02));
}

bool address_v6::is_multicast_node_local() const noexcept
{
    return ((raw_data_[0] == 0xff) && ((raw_data_[1] & 0x0f) == 0x01));
}

bool address_v6::is_multicast_org_local() const noexcept
{
    return ((raw_data_[0] == 0xff) && ((raw_data_[1] & 0x0f) == 0x08));
}

bool address_v6::is_multicast_site_local() const noexcept
{
    return ((raw_data_[0] == 0xff) && ((raw_data_[1] & 0x0f) == 0x05));
}

address_v6 address_v6::any()
{
    return address_v6();
}

address_v6 address_v6::loopback()
{
    address_v6 tmp;
    tmp.raw_data_[15] = 1;
    return tmp;
}

bool operator==(const address_v6& a1, const address_v6& a2)
{
    return a1.raw_data_ == a2.raw_data_;
}

bool operator!=(const address_v6& a1, const address_v6& a2)
{
    return !(a1 == a2);
}

bool operator<(const address_v6& a1, const address_v6& a2)
{
    return a1.raw_data_ < a2.raw_data_;
}

bool operator>(const address_v6& a1, const address_v6& a2)
{
    return a2 < a1;
}

bool operator<=(const address_v6& a1, const address_v6& a2)
{
    return !(a2 < a1);
}

bool operator>=(const address_v6& a1, const address_v6& a2)
{
    return !(a1 < a2);
}

address_v6 make_address_v6(const address_v6::bytes_type& bytes, unsigned long scope_id) noexcept
{
    return address_v6{bytes, scope_id};
}

}
}


