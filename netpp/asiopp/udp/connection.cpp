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
	auto& service = socket_->get_io_service();
	service.post(strand_->wrap([this, shared_this = shared_from_this()]() {

		if(stopped())
		{
			return;
		}

		// Start an asynchronous operation to read a certain number of bytes.
		// Here std::bind + shared_from_this is used because of the composite op async_*
		// We want it to operate on valid data until the handler is called.
		socket_->async_receive_from(asio::buffer(input_buffer_.buffer.data() + input_buffer_.offset,
												 input_buffer_.buffer.size() - input_buffer_.offset),
									remote_endpoint_,
									strand_->wrap(std::bind(&base_type::handle_read, shared_this,
															std::placeholders::_1, std::placeholders::_2)));
	}));
}
int64_t udp_connection::handle_read(const error_code& ec, std::size_t size)
{
	auto unprocessed_size = input_buffer_.offset + size;
	auto result = handle_read(ec, input_buffer_.buffer.data(), unprocessed_size);
	if(result < 0)
	{
		return result;
	}
	auto processed = static_cast<std::size_t>(result);
	auto unprocessed = unprocessed_size - processed;

	if(processed > unprocessed)
	{
		std::memcpy(input_buffer_.buffer.data(), input_buffer_.buffer.data() + processed, unprocessed);
	}
	else
	{
		byte_buffer range(unprocessed);
		std::memcpy(range.data(), input_buffer_.buffer.data() + processed, unprocessed);
		std::memcpy(input_buffer_.buffer.data(), range.data(), range.size());
	}

	input_buffer_.offset = unprocessed;

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

} // namespace udp
} // namespace net
