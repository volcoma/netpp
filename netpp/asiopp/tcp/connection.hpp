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
void tcp_connection<socket_type>::start_read()
{
	std::lock_guard<std::mutex> lock(this->guard_);
	// Start an asynchronous operation to read a certain number of bytes.
	auto operation = this->msg_builder_->get_next_operation();
	auto& work_buffer = this->msg_builder_->get_work_buffer();
	auto offset = work_buffer.size();
	work_buffer.resize(offset + operation.bytes);

    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
	asio::async_read(*this->socket_, asio::buffer(work_buffer.data() + offset, operation.bytes),
					 asio::transfer_exactly(operation.bytes),
					 this->strand_.wrap(std::bind(&base_type::handle_read, this->shared_from_this(),
											std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type>
void tcp_connection<socket_type>::handle_read(const error_code& ec, std::size_t size)
{
	if(this->stopped())
	{
		return;
	}
	if(size == 0)
	{
		this->start();
		return;
	}

	if(!ec)
	{
		auto extract_msg = [&]() -> byte_buffer {
			std::unique_lock<std::mutex> lock(this->guard_);
			bool is_ready = this->msg_builder_->process_operation(size);
			if(is_ready)
			{
				// Extract the message from the builder.
				return this->msg_builder_->extract_msg();
			}

			return {};
		};

		auto msg = extract_msg();

		start_read();

		if(!msg.empty())
		{
			this->on_msg(this->id, msg);
		}
	}
	else
	{
		this->stop(ec);
	}
}

template <typename socket_type>
void tcp_connection<socket_type>::start_write()
{
	std::lock_guard<std::mutex> lock(this->guard_);
	const auto& to_wire = this->output_queue_.front();
	std::vector<asio::const_buffer> buffers;
	buffers.reserve(to_wire.size());
	for(const auto& buf : to_wire)
	{
		buffers.emplace_back(asio::buffer(buf));
	}

	// Start an asynchronous operation to send all messages.

    // Here std::bind + shared_from_this is used because of the composite op async_*
    // We want it to operate on valid data until the handler is called.
	asio::async_write(*this->socket_, buffers,
					  this->strand_.wrap(std::bind(&base_type::handle_write, this->shared_from_this(),
											 std::placeholders::_1)));
}
template <typename socket_type>
void tcp_connection<socket_type>::handle_write(const error_code& ec)
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
