#pragma once
#include <netpp/connection.h>

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
//----------------------------------------------------------------------
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
// Once a message is available, it is sent to the client. After the message is
// successfully sent, the output actor again waits for the output queue to
// become non-empty.

template <typename socket_type>
class asio_connection : public connection, public std::enable_shared_from_this<asio_connection<socket_type>>
{
public:
	//-----------------------------------------------------------------------------
	/// Constructor of connection accepting a ready socket.
	//-----------------------------------------------------------------------------
	asio_connection(std::shared_ptr<socket_type> socket, const msg_builder::creator& builder_creator,
					asio::io_service& context);
	//-----------------------------------------------------------------------------
	/// Starts the connection. Awaiting input and output
	//-----------------------------------------------------------------------------
	void start() override;

	//-----------------------------------------------------------------------------
	/// Stops the connection with the specified error code.
	/// Can be called internally from a failed async operation
	//-----------------------------------------------------------------------------
	void stop(const error_code& ec) override;

	//-----------------------------------------------------------------------------
	/// Starts the async read operation awaiting for data
	/// to be read from the socket.
	//-----------------------------------------------------------------------------
	virtual void start_read() = 0;

	//-----------------------------------------------------------------------------
	/// Callback to be called whenever data was read from the socket
	/// or an error occured.
	//-----------------------------------------------------------------------------
	virtual void handle_read(const error_code& ec, std::size_t n) = 0;

	//-----------------------------------------------------------------------------
	/// Starts the async write operation awaiting for data
	/// to be written to the socket.
	//-----------------------------------------------------------------------------
	virtual void start_write() = 0;

	//-----------------------------------------------------------------------------
	/// Callback to be called whenever data was written to the socket
	/// or an error occured.
	//-----------------------------------------------------------------------------
	virtual void handle_write(const error_code& ec) = 0;

protected:
	//-----------------------------------------------------------------------------
	/// Checks whether the connection is stopped i.e the stop method
	/// has been called at least once.
	//-----------------------------------------------------------------------------
	bool stopped() const;

	//-----------------------------------------------------------------------------
	/// Sends the message through the specified channel
	//-----------------------------------------------------------------------------
	void send_msg(byte_buffer&& msg, data_channel channel) override;

	//-----------------------------------------------------------------------------
	/// Awaits for output in the output queue. If the output queue is
	/// not empty then the start_write function will be called.
	//-----------------------------------------------------------------------------
	void await_output();

	/// guard for shared data access
	mutable std::mutex guard_;

	/// deque to avoid elements invalidation when resizing
	/// Access to this member should be guarded by a lock
	std::deque<std::vector<byte_buffer>> output_queue_;

	/// a strand for async socket callback synchronization
	asio::io_service::strand strand_;

	/// the socket this connection is using.
	std::shared_ptr<socket_type> socket_;

	/// steady_timer as an asynchronous condition variable.
	/// The steady_timer will be signalled whenever the
	/// output queue is non-empty.
	/// Access to this member should be guarded by a lock
	asio::steady_timer non_empty_output_queue_;

	/// a security flag to tell us if we are still connected.
	std::atomic<bool> connected_{false};
};

template <typename socket_type>
inline asio_connection<socket_type>::asio_connection(std::shared_ptr<socket_type> socket,
													 const msg_builder::creator& builder_creator,
													 asio::io_service& context)
	: strand_(context)
	, socket_(std::move(socket))
	, non_empty_output_queue_(context)
{
	socket_->lowest_layer().non_blocking(true);
	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());

	builder = builder_creator();
}

template <typename socket_type>
inline void asio_connection<socket_type>::start()
{
	connected_ = true;
	start_read();
	await_output();
}
template <typename socket_type>
inline void asio_connection<socket_type>::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();

		auto& service = socket_->get_io_service();
		service.post(strand_.wrap([socket = socket_]() {
			if(socket->lowest_layer().is_open())
			{
				error_code ec;
				socket->lowest_layer().shutdown(asio::socket_base::shutdown_both, ec);
				socket->lowest_layer().close(ec);
			}
		}));
	}
	if(connected_.exchange(false))
	{
		for(const auto& callback : on_disconnect)
		{
			callback(id, ec ? ec : asio::error::make_error_code(asio::error::connection_aborted));
		}
	}
}

template <typename socket_type>
inline bool asio_connection<socket_type>::stopped() const
{
	return !connected_;
}

template <typename socket_type>
inline void asio_connection<socket_type>::send_msg(byte_buffer&& msg, data_channel channel)
{
	// we assume this is thread safe as it is const.
	auto buffers = builder->build(std::move(msg), channel);

	std::lock_guard<std::mutex> lock(guard_);
	output_queue_.emplace_back(std::move(buffers));

	// Signal that the output queue contains messages. Modifying the expiry
	// will wake the output actor, if it is waiting on the timer.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::min());
}

template <typename socket_type>
inline void asio_connection<socket_type>::await_output()
{
	if(stopped())
	{
		return;
	}

	auto check_if_empty = [&]() {
		std::lock_guard<std::mutex> lock(guard_);
		bool empty = output_queue_.empty();
		if(empty)
		{
			// There are no messages that are ready to be sent. The actor goes to
			// sleep by waiting on the non_empty_output_queue_ timer. When a new
			// message is added, the timer will be modified and the actor will wake.
			non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());
			non_empty_output_queue_.async_wait(
				strand_.wrap(std::bind(&asio_connection::await_output, this->shared_from_this())));
		}

		return empty;
	};

	if(!check_if_empty())
	{
		start_write();
	}
}

} // namespace net
