#pragma once
#include "../common/connection.hpp"

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <chrono>
#include <deque>
#include <thread>

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
    bool handle_read(const error_code& ec, std::size_t size) override;

    //-----------------------------------------------------------------------------
    /// Starts the async write operation awaiting for data
    /// to be written to the socket.
    //-----------------------------------------------------------------------------
    void start_write() override;

private:
    /// Endpoint used for sending
    udp::endpoint endpoint_;

    /// Remote endpoint filled when recieving
    udp::endpoint remote_endpoint_;

    /// Input buffer used when recieving
    input_buffer input_buffer_{};
};
} // namespace udp
} // namespace net
