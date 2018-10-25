#include "connection.h"

namespace net
{
namespace udp
{

void udp_connection::set_endpoint(udp::endpoint endpoint, bool can_read, bool can_write)
{
    endpoint_ = std::move(endpoint);
    can_read_ = can_read;
    can_write_ = can_write;
}

void udp_connection::start_read()
{
    if(!can_read_)
    {
        return;
    }
    // Start an asynchronous operation to read a certain number of bytes.
    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
    socket_->async_receive_from(asio::null_buffers(), remote_endpoint_, asio::socket_base::message_peek,
                                strand_.wrap(std::bind(&base_type::handle_read, this->shared_from_this(),
                                                       std::placeholders::_1, std::placeholders::_2)));
}
void udp_connection::handle_read(const error_code& ec, std::size_t)
{
    if(stopped())
    {
        return;
    }

    if(!ec)
    {
        using namespace std::chrono_literals;

        while(true)
        {
            if(stopped())
            {
                return;
            }

            error_code av;
            auto available = socket_->lowest_layer().available(av);

            if(av)
            {
                stop(av);
                return;
            }

            if(available == 0)
            {
                std::this_thread::yield();
                break;
            }

            byte_buffer buf(available);
            if(socket_->receive(asio::buffer(buf)) == 0)
            {
                std::this_thread::yield();
                break;
            }

            size_t processed = 0;
            while(processed < available)
            {
                // NOTE! Thread safety:
                // the builder should only be used for reads
                // which are already synchronized via the explicit 'strand'
                auto operation = builder->get_next_operation();

                auto left = available - processed;
                if(left < operation.bytes)
                {
                    break;
                }

                auto& work_buffer = builder->get_work_buffer();
                auto offset = work_buffer.size();
                work_buffer.resize(offset + operation.bytes);
                std::memcpy(work_buffer.data() + offset, buf.data() + processed, operation.bytes);

                bool is_ready = false;
                try
                {
                    is_ready = builder->process_operation(operation.bytes);
                }
                catch(...)
                {
                    stop(make_error_code(std::errc::illegal_byte_sequence));
                    return;
                }

                if(is_ready)
                {
                    // Extract the message from the builder.
                    auto msg_data = builder->extract_msg();
                    auto& msg = msg_data.first;
                    auto channel = msg_data.second;

                    for(const auto& callback : on_msg)
                    {
                        callback(id, msg, channel);
                    }
                }

                processed += operation.bytes;
            }
        }

        start_read();
    }
    else
    {
        stop(ec);
    }
}

void udp_connection::start_write()
{
    if(!can_write_)
    {
        std::lock_guard<std::mutex> lock(guard_);
        output_queue_.clear();
        return;
    }

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
    socket_->async_send_to(
        buffers, endpoint_,
        strand_.wrap(std::bind(&base_type::handle_write, this->shared_from_this(), std::placeholders::_1)));
}
void udp_connection::handle_write(const error_code& ec)
{
    if(stopped())
    {
        return;
    }
    if(!ec)
    {
        {
            std::lock_guard<std::mutex> lock(guard_);
            output_queue_.pop_front();
        }
        await_output();
    }
    else
    {
        stop(ec);
    }
}
}
} // namespace net
