#pragma once
#include "../common/connection.hpp"

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_service.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <chrono>
#include <deque>
#include <thread>

namespace net
{
namespace tcp
{

template <typename socket_type>
class tcp_connection : public asio_connection<socket_type>
{
public:
    //-----------------------------------------------------------------------------
    /// Aliases.
    //-----------------------------------------------------------------------------
    using base_type = asio_connection<socket_type>;

    //-----------------------------------------------------------------------------
    /// Inherited constructors
    //-----------------------------------------------------------------------------
    using base_type::base_type;

    //-----------------------------------------------------------------------------
    /// Starts the async read operation awaiting for data
    /// to be read from the socket.
    //-----------------------------------------------------------------------------
    void start_read() override;

    //-----------------------------------------------------------------------------
    /// Callback to be called whenever data was read from the socket
    /// or an error occured.
    //-----------------------------------------------------------------------------
    void handle_read(const error_code& ec, std::size_t n) override;

    //-----------------------------------------------------------------------------
    /// Starts the async write operation awaiting for data
    /// to be written to the socket.
    //-----------------------------------------------------------------------------
    void start_write() override;

    //-----------------------------------------------------------------------------
    /// Callback to be called whenever data was written to the socket
    /// or an error occured.
    //-----------------------------------------------------------------------------
    void handle_write(const error_code& ec) override;
};

template <typename socket_type>
inline void tcp_connection<socket_type>::start_read()
{
    // NOTE! Thread safety:
    // the builder should only be used for reads
    // which are already synchronized via the explicit 'strand'

    auto operation = this->builder->get_next_operation();
    auto& work_buffer = this->builder->get_work_buffer();
    auto offset = work_buffer.size();
    work_buffer.resize(offset + operation.bytes);

    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
    // Start an asynchronous operation to read a certain number of bytes.
    asio::async_read(*this->socket_, asio::buffer(work_buffer.data() + offset, operation.bytes),
                     asio::transfer_exactly(operation.bytes),
                     this->strand_.wrap(std::bind(&base_type::handle_read, this->shared_from_this(),
                                                  std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type>
inline void tcp_connection<socket_type>::handle_read(const error_code& ec, std::size_t size)
{
    if(this->stopped())
    {
        return;
    }
    if(!ec)
    {
        // NOTE! Thread safety:
        // the builder should only be used for reads
        // which are already synchronized via the explicit 'strand'

        bool is_ready = false;
        try
        {
            is_ready = this->builder->process_operation(size);
        }
        catch(...)
        {
            this->stop(make_error_code(std::errc::illegal_byte_sequence));
            return;
        }

        if(is_ready)
        {
            // Extract the message from the builder.
            auto msg_data = this->builder->extract_msg();
            auto& msg = msg_data.first;
            auto channel = msg_data.second;

            if(!msg.empty())
            {
                for(const auto& callback : this->on_msg)
                {
                    callback(this->id, msg, channel);
                }
            }
        }
        start_read();
    }
    else
    {
        this->stop(ec);
    }
}

template <typename socket_type>
inline void tcp_connection<socket_type>::start_write()
{
    auto buffers = [&]() {
        std::lock_guard<std::mutex> lock(this->guard_);
        const auto& to_wire = this->output_queue_.front();
        std::vector<asio::const_buffer> buffers;
        buffers.reserve(to_wire.size());
        for(const auto& buf : to_wire)
        {
            buffers.emplace_back(asio::buffer(buf));
        }
        return buffers;
    }();

    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
    // Start an asynchronous operation to send all messages.
    asio::async_write(*this->socket_, buffers,
                      this->strand_.wrap(std::bind(&base_type::handle_write, this->shared_from_this(),
                                                   std::placeholders::_1)));
}
template <typename socket_type>
inline void tcp_connection<socket_type>::handle_write(const error_code& ec)
{
    if(this->stopped())
    {
        return;
    }
    if(!ec)
    {
        {
            std::lock_guard<std::mutex> lock(this->guard_);
            this->output_queue_.pop_front();
        }
        this->await_output();
    }
    else
    {
        this->stop(ec);
    }
}
}
} // namespace net
