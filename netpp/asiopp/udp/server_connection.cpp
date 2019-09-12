#include "server_connection.h"

namespace net
{
namespace udp
{

void udp_server_connection::set_options(udp::endpoint endpoint, std::shared_ptr<asio::io_service::strand> strand)
{
    endpoint_ = std::move(endpoint);
    strand_ = std::move(strand);
}


void udp_server_connection::stop_socket()
{
}

void udp_server_connection::start_read()
{
}

size_t udp_server_connection::process_read(const uint8_t* buf, std::size_t n)
{
    size_t processed = 0;
    while(processed < n)
    {
        // NOTE! Thread safety:
        // the builder should only be used for reads
        // which are already synchronized via the explicit 'strand'
        auto operation = builder->get_next_operation();

        auto left = n - processed;
        if(left < operation.bytes)
        {
            break;
        }

        auto& work_buffer = builder->get_work_buffer();
        auto offset = work_buffer.size();
        work_buffer.resize(offset + operation.bytes);
        std::memcpy(work_buffer.data() + offset, buf + processed, operation.bytes);

        if(!base_type::handle_read({}, operation.bytes))
        {
            return processed;
        }

        processed += operation.bytes;
    }

    return processed;
}


void udp_server_connection::start_write()
{
    auto& service = socket_->get_io_service();
    service.post(strand_->wrap([this, shared_this = shared_from_this()]() {
        if(stopped())
        {
            return;
        }
        // Here std::bind + shared_from_this is used because of the composite op async_*
        // We want it to operate on valid data until the handler is called.
        // Start an asynchronous operation to send all messages.
        socket_->async_send_to(this->get_output_buffers(), this->endpoint_,
                               strand_->wrap(std::bind(&base_type::handle_write, shared_this,
                                                      std::placeholders::_1, std::placeholders::_2)));
    }));

}
}
} // namespace net
