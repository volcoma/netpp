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
	socket_->async_receive_from(
		asio::buffer(input_buffer_.buffer.data() + input_buffer_.offset,
		input_buffer_.buffer.size() - input_buffer_.offset),
		remote_endpoint_,
		strand_.wrap(std::bind(&base_type::handle_read, this->shared_from_this(), std::placeholders::_1,
							   std::placeholders::_2)));
}
bool udp_connection::handle_read(const error_code& ec, std::size_t size)
{
    size += input_buffer_.offset;
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
		std::memcpy(work_buffer.data() + offset, input_buffer_.buffer.data() + processed, operation.bytes);

		if(!base_type::handle_read(ec, operation.bytes))
		{
			return false;
		}

		processed += operation.bytes;
	}

	input_buffer_.offset = size - processed;
	start_read();
	return true;
}

void udp_connection::start_write()
{
	if(!can_write_)
	{
		std::lock_guard<std::mutex> lock(guard_);
		output_queue_.clear();
		return;
	}

	// Here std::bind + shared_from_this is used because of the composite op async_*
	// We want it to operate on valid data until the handler is called.
	// Start an asynchronous operation to send all messages.
	socket_->async_send_to(this->get_output_buffers(), endpoint_,
						   strand_.wrap(std::bind(&base_type::handle_write, this->shared_from_this(),
												  std::placeholders::_1, std::placeholders::_2)));
}

} // namespace udp
} // namespace net
