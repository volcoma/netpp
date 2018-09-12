#pragma once
#include "../channel.h"
#include "../msg_builder.hpp"

#include <asio/basic_stream_socket.hpp>
#include <asio/error.hpp>
#include <asio/strand.hpp>
#include <mutex>
namespace net
{
using asio::steady_timer;

//----------------------------------------------------------------------

//
// This class manages socket timeouts by applying the concept of a deadline.
// Some asynchronous operations are given deadlines by which they must complete.
// Deadlines are enforced by two "actors" that persist for the lifetime of the
// session object, one for input and one for output:
//
//  +----------------+                     +----------------+
//  |                |                     |                |
//  | check_deadline |<---+                | check_deadline |<---+
//  |                |    | async_wait()   |                |    | async_wait()
//  +----------------+    |  on input      +----------------+    |  on output
//              |         |  deadline                  |         |  deadline
//              +---------+                            +---------+
//
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

template <typename socket_type>
class tcp_connection : public connection, public std::enable_shared_from_this<tcp_connection<socket_type>>
{
public:
	tcp_connection(std::shared_ptr<socket_type> socket, asio::io_context& io_context);

	void start() override;
	void stop(const asio::error_code& ec) override;

private:
	bool stopped() const;
	void send_msg(byte_buffer&& msg) override;
	void start_read();
	void handle_read(const asio::error_code& ec, std::size_t n);
	void await_output();
	void start_write();
	void handle_write(const asio::error_code& ec);
	void check_deadline(steady_timer* deadline, io_context::strand* strand);

	mutable std::mutex guard_;
	msg_builder msg_builder_;
	io_context::strand reader_;
	io_context::strand writer_;
	std::shared_ptr<socket_type> socket_;
	std::deque<byte_buffer> output_queue_;
	steady_timer non_empty_output_queue_;
	std::atomic<bool> connected{false};
};

template <typename socket_type>
tcp_connection<socket_type>::tcp_connection(std::shared_ptr<socket_type> socket, asio::io_context& io_context)
	: msg_builder_(sizeof(uint32_t))
	, reader_(io_context)
	, writer_(io_context)
	, socket_(socket)
	, non_empty_output_queue_(io_context)
{

	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(steady_timer::time_point::max());
}

template <typename socket_type>
void tcp_connection<socket_type>::start()
{
	connected = true;
	start_read();
	await_output();
	on_connect_(this->shared_from_this());
}
template <typename socket_type>
void tcp_connection<socket_type>::stop(const asio::error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();
	}

	if(connected)
	{
		connected = false;
		on_disconnect_(this->shared_from_this(), ec);
	}
}
template <typename socket_type>
bool tcp_connection<socket_type>::stopped() const
{
	std::lock_guard<std::mutex> lock(guard_);
	//return !socket_->lowest_layer().is_open();
	return !connected;
}

template <typename socket_type>
void tcp_connection<socket_type>::send_msg(byte_buffer&& msg)
{
	auto msg_size = static_cast<uint32_t>(msg.size());
	size_t sizeof_size = sizeof(msg_size);

	byte_buffer output_msg;
	output_msg.reserve(sizeof_size + msg_size);
	for(size_t i = 0; i < sizeof_size; ++i)
	{
		output_msg.push_back(char(msg_size >> (i * 8)));
	}
	std::copy(std::begin(msg), std::end(msg), std::back_inserter(output_msg));

	std::lock_guard<std::mutex> lock(guard_);
	output_queue_.emplace_back(std::move(output_msg));

	// Signal that the output queue contains messages. Modifying the expiry
	// will wake the output actor, if it is waiting on the timer.
	non_empty_output_queue_.expires_at(steady_timer::time_point::min());
}
template <typename socket_type>
void tcp_connection<socket_type>::start_read()
{
	std::lock_guard<std::mutex> lock(guard_);

	// Start an asynchronous operation to read a certain number of bytes.
	auto expected = msg_builder_.get_next_read();
	asio::async_read(*socket_, asio::dynamic_buffer(msg_builder_.buffer), asio::transfer_exactly(expected),
					 reader_.wrap(std::bind(&tcp_connection::handle_read, this->shared_from_this(),
											std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type>
void tcp_connection<socket_type>::handle_read(const asio::error_code& ec, std::size_t)
{
	if(stopped())
	{
		return;
	}

	if(!ec)
	{
		{

			std::unique_lock<std::mutex> lock(guard_);

			// Extract the message from the buffer.
			msg_builder_.read_chunk();
			if(msg_builder_.is_ready())
			{
				auto msg = std::move(msg_builder_.buffer);
				msg_builder_.clear();

				lock.unlock();

				on_msg_(this->shared_from_this(), msg);
			}
		}

		start_read();
	}
	else
	{
		stop(ec);
	}
}
template <typename socket_type>
void tcp_connection<socket_type>::await_output()
{
	if(stopped())
	{
		return;
	}
	std::unique_lock<std::mutex> lock(guard_);

	if(output_queue_.empty())
	{
		// There are no messages that are ready to be sent. The actor goes to
		// sleep by waiting on the non_empty_output_queue_ timer. When a new
		// message is added, the timer will be modified and the actor will wake.
		non_empty_output_queue_.expires_at(steady_timer::time_point::max());
		non_empty_output_queue_.async_wait(
			writer_.wrap(std::bind(&tcp_connection::await_output, this->shared_from_this())));
	}
	else
	{
		lock.unlock();
		start_write();
	}
}
template <typename socket_type>
void tcp_connection<socket_type>::start_write()
{
	std::lock_guard<std::mutex> lock(guard_);

	// Start an asynchronous operation to send a message.
	asio::async_write(*socket_, asio::buffer(output_queue_.front()),
					  writer_.wrap(std::bind(&tcp_connection::handle_write, this->shared_from_this(),
											 std::placeholders::_1)));
}
template <typename socket_type>
void tcp_connection<socket_type>::handle_write(const asio::error_code& ec)
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
