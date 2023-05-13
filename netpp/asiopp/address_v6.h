#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace net
{

namespace ip
{

class address_v6
{
public:
    using bytes_type = std::array<uint8_t, 16>;

    // Default constructor.
    address_v6() noexcept = default;

    /// Construct an address from raw bytes and scope ID.
    explicit address_v6(const bytes_type& bytes, unsigned long scope_id = 0) noexcept;

    /// Copy constructor.
    address_v6(const address_v6& other) noexcept = default;

    /// Move constructor
    address_v6(address_v6&& other) noexcept = default;

    address_v6& operator=(const address_v6& other) noexcept = default;
    address_v6& operator=(address_v6&& other) noexcept = default;

    /// Get the address in bytes, in network byte order.
    bytes_type to_bytes() const noexcept;

    /// Get the address as a string.
    std::string to_string() const;

    /// The scope ID of the address.
    /**
     * Returns the scope ID associated with the IPv6 address.
     */
    unsigned long scope_id() const
    {
        return scope_id_;
    }

    /// The scope ID of the address.
    /**
     * Modifies the scope ID associated with the IPv6 address.
     */
    void scope_id(unsigned long id)
    {
        scope_id_ = id;
    }

    /// Determine whether the address is a loopback address.
    bool is_loopback() const noexcept;

    /// Determine whether the address is unspecified.
    bool is_unspecified() const noexcept;

    /// Determine whether the address is link local.
    bool is_link_local() const noexcept;

    /// Determine whether the address is site local.
    bool is_site_local() const noexcept;

    /// Determine whether the address is a mapped IPv4 address.
    bool is_v4_mapped() const noexcept;

    /// Determine whether the address is a multicast address.
    bool is_multicast() const noexcept;

    /// Determine whether the address is a global multicast address.
    bool is_multicast_global() const noexcept;

    /// Determine whether the address is a link-local multicast address.
    bool is_multicast_link_local() const noexcept;

    /// Determine whether the address is a node-local multicast address.
    bool is_multicast_node_local() const noexcept;

    /// Determine whether the address is a org-local multicast address.
    bool is_multicast_org_local() const noexcept;

    /// Determine whether the address is a site-local multicast address.
    bool is_multicast_site_local() const noexcept;

    /// Obtain an address object that represents any address.
    static address_v6 any();

    /// Obtain an address object that represents the loopback address.
    static address_v6 loopback();

    /// Compare two addresses for equality.
    friend bool operator==(const address_v6& a1, const address_v6& a2);

    /// Compare two addresses for inequality.
    friend bool operator!=(const address_v6& a1, const address_v6& a2);

    /// Compare addresses for ordering.
    friend bool operator<(const address_v6& a1, const address_v6& a2);

    /// Compare addresses for ordering.
    friend bool operator>(const address_v6& a1, const address_v6& a2);

    /// Compare addresses for ordering.
    friend bool operator<=(const address_v6& a1, const address_v6& a2);

    /// Compare addresses for ordering.
    friend bool operator>=(const address_v6& a1, const address_v6& a2);

private:
    bytes_type raw_data_ {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    unsigned long scope_id_ {};
};

address_v6 make_address_v6(const address_v6::bytes_type& bytes, unsigned long scope_id = 0) noexcept;

}

}
