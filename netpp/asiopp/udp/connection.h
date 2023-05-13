#pragma once
#include "../common/connection.hpp"

#include <asio/ip/udp.hpp>
#include <map>

namespace net
{
namespace udp
{

using asio::ip::udp;

class udp_connection : public asio_connection<udp::socket>
{
public:
    //-----------------------------------------------------------------------------
    /// Aliases.
    //-----------------------------------------------------------------------------
    using base_type = asio_connection<udp::socket>;

    //-----------------------------------------------------------------------------
    /// Inherited constructors
    //-----------------------------------------------------------------------------
    using base_type::base_type;

    //-----------------------------------------------------------------------------
    /// Sets and endpoint and read/write rights
    //-----------------------------------------------------------------------------
    void set_endpoint(udp::endpoint endpoint);

    //-----------------------------------------------------------------------------
    /// Starts the async read operation awaiting for data
    /// to be read from the socket.
    //-----------------------------------------------------------------------------
    void start_read() override;

    //-----------------------------------------------------------------------------
    /// Callback to be called whenever data was read from the socket
    /// or an error occured.
    //-----------------------------------------------------------------------------
    int64_t handle_read(const error_code& ec, std::size_t size) override;
    int64_t handle_read(const error_code& ec, const uint8_t* buf, std::size_t size);

    //-----------------------------------------------------------------------------
    /// Starts the async write operation awaiting for data
    /// to be written to the socket.
    //-----------------------------------------------------------------------------
    void start_write() override;

private:

    /// Input buffer used when recieving
    raw_buffer input_buffer_{};
    std::map<socket_endpoint, output_buffer> input_buffers_ {};
};
} // namespace udp
} // namespace net
