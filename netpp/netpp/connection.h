#pragma once

#include "msg_builder.h"
#include "error_code.h"

#include <functional>
#include <memory>
#include <string>
#include <deque>

namespace net
{
//----------------------------------------------------------------------

struct connection
{
    struct details
    {
        // Local endpoint(you).
        std::string local_endpoint {};

        // Remote sender endpoint.
        // Endpoint which sends to you.
        std::string remote_endpoint {};

        // Your send endpoint.
        std::string endpoint {};
    };

    //-----------------------------------------------------------------------------
    /// Aliases
    //-----------------------------------------------------------------------------
    using id_t = uint64_t;
    using on_disconnect_t = std::function<void(connection::id_t, const error_code&)>;
    using on_msg_t = std::function<void(connection::id_t, byte_buffer, data_channel, const details&)>;

    connection();
    virtual ~connection() = default;

    //-----------------------------------------------------------------------------
    /// Sends the message through the specified channel
    //-----------------------------------------------------------------------------
    virtual void send_msg(byte_buffer&& msg, data_channel channel) = 0;

    //-----------------------------------------------------------------------------
    /// Starts the connection.
    //-----------------------------------------------------------------------------
    virtual void start() = 0;

    //-----------------------------------------------------------------------------
    /// Stops the connection with the specified error code.
    //-----------------------------------------------------------------------------
    virtual void stop(const error_code& ec) = 0;

    /// container of subscribers for on_msg
    std::deque<on_msg_t> on_msg;

    /// container of subscribers for on_disconnect
    std::deque<on_disconnect_t> on_disconnect;

    /// unique msg_builder for this connection
    msg_builder_ptr builder;

    /// connection id
    const id_t id;
};
using connection_ptr = std::shared_ptr<connection>;

} // namespace net
