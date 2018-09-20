#include <utility>

#include <utility>

#include "connection.h"

namespace net
{
namespace udp
{

async_connection::async_connection(std::shared_ptr<udp::socket> socket, udp::endpoint endpoint,
								   asio::io_service& context, bool can_read, bool can_write)
	: strand_(context)
	, endpoint_(std::move(endpoint))
	, socket_(std::move(socket))
	, non_empty_output_queue_(context)
	, can_read_(can_read)
	, can_write_(can_write)
{
	socket_->lowest_layer().non_blocking(true);
	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());

	msg_builder_ = std::make_unique<multi_buffer_builder>();
}

void async_connection::start()
{
	connected_ = true;
	start_read();
	await_output();
}

void async_connection::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();
	}

	if(connected_.exchange(false))
	{
		for(const auto& callback : on_disconnect)
		{
			callback(id, ec ? ec : asio::error::make_error_code(asio::error::connection_aborted));
		}
	}
}
bool async_connection::stopped() const
{
	std::lock_guard<std::mutex> lock(guard_);
	return !socket_->lowest_layer().is_open();
}

void async_connection::send_msg(byte_buffer&& msg, data_channel channel)
{
	if(!can_write_)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(guard_);
	auto buffers = msg_builder_->build(std::move(msg), channel);

	output_queue_.emplace_back(std::move(buffers));

	// Signal that the output queue contains messages. Modifying the expiry
	// will wake the output actor, if it is waiting on the timer.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::min());
}

void async_connection::start_read()
{
	if(!can_read_)
	{
		return;
	}
	// Start an asynchronous operation to read a certain number of bytes.
	socket_->async_receive_from(
		asio::null_buffers(), remote_endpoint_, asio::socket_base::message_peek,
		strand_.wrap(std::bind(&async_connection::handle_read, this->shared_from_this(),
							   std::placeholders::_1, std::placeholders::_2)));
}
void async_connection::handle_read(const error_code& ec, std::size_t)
{
	if(stopped())
	{
		return;
	}

	if(!ec)
	{
		using namespace std::chrono_literals;
		std::unique_lock<std::mutex> lock(guard_);

		while(true)
		{
			auto available = socket_->lowest_layer().available();
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
				// Start an asynchronous operation to read a certain number of bytes.
				auto operation = msg_builder_->get_next_operation();

				auto left = available - processed;
				if(left < operation.bytes)
				{
					break;
				}

				auto& work_buffer = msg_builder_->get_work_buffer();
				auto offset = work_buffer.size();
				work_buffer.resize(offset + operation.bytes);
				std::memcpy(work_buffer.data(), buf.data() + processed, work_buffer.size());

				bool is_ready = msg_builder_->process_operation(operation.bytes);
				if(is_ready)
				{
					// Extract the message from the builder.
					auto msg = msg_builder_->extract_msg();
					if(on_msg)
					{
						lock.unlock();

						on_msg(id, msg);

						lock.lock();
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

void async_connection::await_output()
{
	if(!can_write_)
	{
		return;
	}
	if(stopped())
	{
		return;
	}

	auto check_if_empty = [&]() {
		std::unique_lock<std::mutex> lock(guard_);
		bool empty = output_queue_.empty();
		if(empty)
		{
			// There are no messages that are ready to be sent. The actor goes to
			// sleep by waiting on the non_empty_output_queue_ timer. When a new
			// message is added, the timer will be modified and the actor will wake.
			non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());
			non_empty_output_queue_.async_wait(
				strand_.wrap(std::bind(&async_connection::await_output, this->shared_from_this())));
		}

		return empty;
	};

	if(!check_if_empty())
	{
		start_write();
	}
}
void async_connection::start_write()
{
	std::lock_guard<std::mutex> lock(guard_);
	const auto& to_wire = output_queue_.front();
	std::vector<asio::const_buffer> buffers;
	buffers.reserve(to_wire.size());
	for(const auto& buf : to_wire)
	{
		buffers.emplace_back(asio::buffer(buf));
	}

	// Start an asynchronous operation to send all messages.
	socket_->async_send_to(buffers, endpoint_,
						   strand_.wrap(std::bind(&async_connection::handle_write, this->shared_from_this(),
												  std::placeholders::_1)));
}
void async_connection::handle_write(const error_code& ec)
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
