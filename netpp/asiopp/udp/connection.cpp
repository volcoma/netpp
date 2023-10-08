#include "connection.h"

namespace net
{
namespace udp
{

void udp_connection::set_endpoint(udp::endpoint endpoint)
{
    endpoint_ = std::move(endpoint);
}

void udp_connection::start_read()
{

    if(stopped())
    {
        return;
    }

    // Start an asynchronous operation to read a certain number of bytes.
    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
    socket_->async_receive_from(asio::buffer(input_buffer_.data(),
                                             input_buffer_.size()),
                                remote_endpoint_,
                                strand_->wrap(std::bind(&base_type::handle_read, this->shared_from_this(),
                                                        std::placeholders::_1, std::placeholders::_2)));
}
int64_t udp_connection::handle_read(const error_code& ec, std::size_t size)
{
    if(size == 0)
    {
        start_read();

        return size;
    }
    auto& input_buffer = input_buffers_[remote_endpoint_];
    auto unprocessed_size = input_buffer.offset + size;
    input_buffer.buffer.resize(unprocessed_size);
    std::memcpy(input_buffer.buffer.data() + input_buffer.offset, input_buffer_.data(), size);

    auto processed = handle_read(ec, input_buffer.buffer.data(), unprocessed_size);
    if(processed < 0)
    {
        input_buffer.offset = 0;

        start_read();

        return processed;
    }

    auto unprocessed = unprocessed_size - static_cast<std::size_t>(processed);

    if(static_cast<std::size_t>(processed) > unprocessed)
    {
        std::memcpy(input_buffer.buffer.data(), input_buffer.buffer.data() + processed, unprocessed);
    }
    else if(unprocessed > 0)
    {
        byte_buffer tmp(unprocessed);
        std::memcpy(tmp.data(), input_buffer.buffer.data() + processed, unprocessed);
        std::memcpy(input_buffer.buffer.data(), tmp.data(), tmp.size());
    }

    input_buffer.offset = unprocessed;

    start_read();

    return processed;
}

int64_t udp_connection::handle_read(const error_code& ec, const uint8_t* buf, std::size_t size)
{
    size_t processed = 0;
    while(processed < size)
    {
        // NOTE! Thread safety:
        // the builder should only be used for reads
        // which are already synchronized via the explicit 'strand'
        auto operation = builder->get_next_operation();

        auto left = size - processed;
        if(left < operation.bytes)
        {
            break;
        }

        auto& work_buffer = builder->get_work_buffer();
        auto offset = work_buffer.size();
        work_buffer.resize(offset + operation.bytes);
        std::memcpy(work_buffer.data() + offset, buf + processed, operation.bytes);

        auto result = base_type::handle_read(ec, operation.bytes);

        if(result < 0)
        {
            return result;
        }

        processed += static_cast<size_t>(result);
    }

    return static_cast<int64_t>(processed);
}

void udp_connection::start_write()
{
    if(stopped())
    {
        return;
    }

    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
    // Start an asynchronous operation to send all messages.
    socket_->async_send_to(this->get_output_buffers(), this->endpoint_,
                           strand_->wrap(std::bind(&base_type::handle_write, this->shared_from_this(),
                                                   std::placeholders::_1, std::placeholders::_2)));
}

} // namespace udp
} // namespace net
