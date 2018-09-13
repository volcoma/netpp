#pragma once
#include <netpp/connection.h>

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <deque>
namespace net
{
using asio::io_context;
using asio::steady_timer;
//----------------------------------------------------------------------
// If either deadline actor determines that the corresponding deadline has
// expired, the socket is closed and any outstanding operations are cancelled.
//
// The input actor reads messages from the socket.
//
//  +------------+
//  |            |
//  | start_read |<---+
//  |            |    |
//  +------------+    |
//          |         |
//          |    +-------------+
//  async_- |    |             |
//  read_() +--->| handle_read |
//               |             |
//               +-------------+
//
// The deadline for receiving a complete message is 30 seconds.
//
// The output actor is responsible for sending messages to the client:
//
//  +--------------+
//  |              |<---------------------+
//  | await_output |                      |
//  |              |<---+                 |
//  +--------------+    |                 |
//      |      |        | async_wait()    |
//      |      +--------+                 |
//      V                                 |
//  +-------------+               +--------------+
//  |             | async_write() |              |
//  | start_write |-------------->| handle_write |
//  |             |               |              |
//  +-------------+               +--------------+
//
// The output actor first waits for an output message to be enqueued. It does
// this by using a steady_timer as an asynchronous condition variable. The
// steady_timer will be signalled whenever the output queue is non-empty.
//
// Once a message is available, it is sent to the client. The deadline for
// sending a complete message is 30 seconds. After the message is successfully
// sent, the output actor again waits for the output queue to become non-empty.

template <typename socket_type, typename builder_type>
class tcp_connection : public connection,
					   public std::enable_shared_from_this<tcp_connection<socket_type, builder_type>>
{
public:
	tcp_connection(std::shared_ptr<socket_type> socket, io_context& context);

	void start() override;
	void stop(const error_code& ec) override;

private:
	bool stopped() const;
	void send_msg(byte_buffer&& msg) override;
	void start_read();
	void handle_read(const error_code& ec, std::size_t n);
	void await_output();
	void start_write();
	void handle_write(const error_code& ec);

	mutable std::mutex guard_;
	builder_type msg_builder_;
	std::deque<byte_buffer> output_queue_;

	io_context::strand strand_;

	std::shared_ptr<socket_type> socket_;
	steady_timer non_empty_output_queue_;

	std::atomic<bool> connected{false};
};

template <typename socket_type, typename builder_type>
tcp_connection<socket_type, builder_type>::tcp_connection(std::shared_ptr<socket_type> socket,
														  io_context& context)
	: strand_(context)
	, socket_(socket)
	, non_empty_output_queue_(context)
{

	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(steady_timer::time_point::max());
}

template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::start()
{
	connected = true;
	start_read();
	await_output();
	on_connect(this->shared_from_this());
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();
        std::error_code ignore;
        socket_->lowest_layer().shutdown(asio::socket_base::shutdown_both, ignore);
        socket_->lowest_layer().close(ignore);
	}

	if(connected.exchange(false))
	{
		on_disconnect(this->shared_from_this(),
					  ec ? ec : asio::error::make_error_code(asio::error::connection_aborted));
	}
}
template <typename socket_type, typename builder_type>
bool tcp_connection<socket_type, builder_type>::stopped() const
{
    std::lock_guard<std::mutex> lock(guard_);
    return !socket_->lowest_layer().is_open();
	//return !connected;
}

template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::send_msg(byte_buffer&& msg)
{
	auto output_msg = builder_type::write_header(msg);
	std::copy(std::begin(msg), std::end(msg), std::back_inserter(output_msg));

	std::lock_guard<std::mutex> lock(guard_);
	output_queue_.emplace_back(std::move(output_msg));

	// Signal that the output queue contains messages. Modifying the expiry
	// will wake the output actor, if it is waiting on the timer.
	non_empty_output_queue_.expires_at(steady_timer::time_point::min());
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::start_read()
{
	std::lock_guard<std::mutex> lock(guard_);

	// Start an asynchronous operation to read a certain number of bytes.
	auto expected = msg_builder_.get_next_read();
	asio::async_read(*socket_, asio::dynamic_buffer(msg_builder_.get_buffer()),
					 asio::transfer_exactly(expected),
					 strand_.wrap(std::bind(&tcp_connection::handle_read, this->shared_from_this(),
											std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::handle_read(const error_code& ec, std::size_t)
{
	if(stopped())
	{
		return;
	}

	if(!ec)
	{
		auto extract_buffer = [&]() -> byte_buffer {
			std::unique_lock<std::mutex> lock(guard_);
			// Extract the message from the buffer.
			msg_builder_.read_chunk();
			if(msg_builder_.is_ready())
			{
				return msg_builder_.extract_buffer();
			}

			return {};
		};

		auto msg = extract_buffer();

		start_read();

		if(!msg.empty())
		{
			on_msg(this->shared_from_this(), msg);
		}
	}
	else
	{
		stop(ec);
	}
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::await_output()
{
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
			non_empty_output_queue_.expires_at(steady_timer::time_point::max());
			non_empty_output_queue_.async_wait(
				strand_.wrap(std::bind(&tcp_connection::await_output, this->shared_from_this())));
		}

		return empty;
	};

	if(!check_if_empty())
	{
		start_write();
	}
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::start_write()
{
	std::lock_guard<std::mutex> lock(guard_);

	// Start an asynchronous operation to send a message.
	asio::async_write(*socket_, asio::buffer(output_queue_.front()),
					  strand_.wrap(std::bind(&tcp_connection::handle_write, this->shared_from_this(),
											 std::placeholders::_1)));
}
template <typename socket_type, typename builder_type>
void tcp_connection<socket_type, builder_type>::handle_write(const error_code& ec)
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

} // namespace net
