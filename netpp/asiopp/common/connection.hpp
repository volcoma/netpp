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
//  read()  +--->| handle_read |
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
// Once a message is available, it is sent to the endpoint. After the message is
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
					asio::io_service& context, std::chrono::seconds heartbeat = std::chrono::seconds(0));
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
	virtual bool handle_read(const error_code& ec, std::size_t size);

	//-----------------------------------------------------------------------------
	/// Starts the async write operation awaiting for data
	/// to be written to the socket.
	//-----------------------------------------------------------------------------
	virtual void start_write() = 0;

	//-----------------------------------------------------------------------------
	/// Callback to be called whenever data was written to the socket
	/// or an error occured.
	//-----------------------------------------------------------------------------
	virtual bool handle_write(const error_code& ec, std::size_t size);

protected:
    template<typename buffer_t>
	struct stateful_buffer
	{
        buffer_t buffer{};
		std::size_t offset{};
	};
	using output_buffer = stateful_buffer<byte_buffer>;
	using input_buffer = stateful_buffer<std::array<uint8_t, std::numeric_limits<uint16_t>::max()>>;

	std::vector<asio::const_buffer> get_output_buffers() const;
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

	//-----------------------------------------------------------------------------
	/// Checks if the heartbeat conditions are respected.
	//-----------------------------------------------------------------------------
	void check_heartbeat();

	//-----------------------------------------------------------------------------
	/// Sends a heartbeat.
	//-----------------------------------------------------------------------------
	void send_heartbeat();

	//-----------------------------------------------------------------------------
	/// Shedules a heartbeat.
	//-----------------------------------------------------------------------------
	void schedule_heartbeat();

	//-----------------------------------------------------------------------------
	/// Awaits for the heartbeat timer.
	//-----------------------------------------------------------------------------
	void await_heartbeat();

	//-----------------------------------------------------------------------------
	/// Updates a heartbeat
	//-----------------------------------------------------------------------------
	void update_heartbeat_timestamp();

	/// guard for shared data access
	mutable std::mutex guard_;

	/// deque to avoid elements invalidation when resizing
	/// Access to this member should be guarded by a lock
	std::deque<output_buffer> output_queue_;

	/// a strand for async socket callback synchronization
	asio::io_service::strand strand_;

	/// the socket this connection is using.
	std::shared_ptr<socket_type> socket_;

	/// steady_timer as an asynchronous condition variable.
	/// The steady_timer will be signalled whenever the
	/// output queue is non-empty.
	/// Access to this member should be guarded by a lock
	asio::steady_timer non_empty_output_queue_;

	/// heartbeat interval
	std::chrono::seconds heartbeat_check_interval_;
	/// a steady timer to tick for heartbeat checks
	asio::steady_timer heartbeat_check_timer_;
	/// a steady timer to issue a heartbeat reply
	asio::steady_timer heartbeat_reply_timer_;
	/// the last time a heartbeat was sent/recieved
	std::atomic<asio::steady_timer::duration> heartbeat_timestamp_{};

	/// a security flag to tell us if we are still connected.
	std::atomic<bool> connected_{false};
};

template <typename socket_type>
inline asio_connection<socket_type>::asio_connection(std::shared_ptr<socket_type> socket,
													 const msg_builder::creator& builder_creator,
													 asio::io_service& context,
													 std::chrono::seconds heartbeat)
	: strand_(context)
	, socket_(std::move(socket))
	, non_empty_output_queue_(context)
	, heartbeat_check_interval_(heartbeat)
	, heartbeat_check_timer_(context)
	, heartbeat_reply_timer_(context)

{
	socket_->lowest_layer().non_blocking(true);
	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());
	heartbeat_check_timer_.expires_at(asio::steady_timer::time_point::max());
	heartbeat_reply_timer_.expires_at(asio::steady_timer::time_point::max());

	builder = builder_creator();
}

template <typename socket_type>
inline void asio_connection<socket_type>::start()
{
	connected_ = true;
	start_read();
	await_output();

	if(heartbeat_check_interval_ > std::chrono::seconds::zero())
	{
		send_heartbeat();
		await_heartbeat();
	}
}
template <typename socket_type>
inline void asio_connection<socket_type>::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();
		heartbeat_check_timer_.cancel();
		heartbeat_reply_timer_.cancel();

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
	for(auto& buffer : buffers)
	{
		output_queue_.emplace_back();
		output_queue_.back().buffer = std::move(buffer);
	}

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

template <typename socket_type>
inline bool asio_connection<socket_type>::handle_read(const error_code& ec, std::size_t size)
{
	if(this->stopped())
	{
		return false;
	}

	if(ec)
	{
		this->stop(ec);
		return false;
	}

	// NOTE! Thread safety:
	// the builder should only be used for reads
	// which are already synchronized via the explicit 'strand'

	bool is_ready = false;
	try
	{
		is_ready = this->builder->process_operation(size);
	}
	catch(const std::exception& e)
	{
		log() << e.what();
		this->stop(make_error_code(errc::data_corruption));
		return false;
	}
	catch(...)
	{
		this->stop(make_error_code(errc::data_corruption));
		return false;
	}

	if(is_ready)
	{
		// Extract the message from the builder.
		auto msg_data = this->builder->extract_msg();
		auto& msg = msg_data.first;
		auto channel = msg_data.second;

		if(!msg.empty())
		{
			for(const auto& callback : this->on_msg)
			{
				callback(this->id, msg, channel);
			}
		}
		else
		{
			schedule_heartbeat();
		}
	}

	return true;
}

template <typename socket_type>
inline bool asio_connection<socket_type>::handle_write(const error_code& ec, std::size_t size)
{
	if(this->stopped())
	{
		return false;
	}

	if(ec)
	{
		this->stop(ec);
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(this->guard_);
		while(size > 0 && !this->output_queue_.empty())
		{
			auto& msg = this->output_queue_.front();

			auto buffer_sz = msg.buffer.size();
			auto left = buffer_sz - msg.offset;
			if(size < left)
			{
				msg.offset += size;
				size = 0;
			}
			else
			{
				size -= left;
				this->output_queue_.pop_front();
			}
		}
	}
	this->await_output();
	return true;
}

template <typename socket_type>
inline std::vector<asio::const_buffer> asio_connection<socket_type>::get_output_buffers() const
{
	std::lock_guard<std::mutex> lock(this->guard_);
	std::vector<asio::const_buffer> buffers;
	buffers.reserve(this->output_queue_.size());
	for(const auto& msg : this->output_queue_)
	{
		buffers.emplace_back(
			asio::buffer(msg.buffer.data() + msg.offset, msg.buffer.size() - msg.offset));
	}
	return buffers;
}

template <typename socket_type>
inline void asio_connection<socket_type>::schedule_heartbeat()
{
	update_heartbeat_timestamp();

	std::lock_guard<std::mutex> lock(this->guard_);

	using namespace std::chrono_literals;
	heartbeat_reply_timer_.expires_after(1s);
	heartbeat_reply_timer_.async_wait([this, sentinel = this->shared_from_this()](const error_code& ec) {
		if(this->stopped())
		{
			return;
		}

		if(!ec)
		{
			this->send_heartbeat();
		}
	});
}

template <typename socket_type>
inline void asio_connection<socket_type>::update_heartbeat_timestamp()
{
	this->heartbeat_timestamp_ = asio::steady_timer::clock_type::now().time_since_epoch();
}

template <typename socket_type>
inline void asio_connection<socket_type>::send_heartbeat()
{
	// send heartbeat
	this->send_msg({}, 0);

	update_heartbeat_timestamp();
}

template <typename socket_type>
inline void asio_connection<socket_type>::await_heartbeat()
{
	if(this->stopped())
	{
		return;
	}

	std::lock_guard<std::mutex> lock(this->guard_);
	// Wait before checking the heartbeat.
	heartbeat_check_timer_.expires_after(heartbeat_check_interval_);
	heartbeat_check_timer_.async_wait([this, sentinel = this->shared_from_this()](const error_code& ec) {
		if(this->stopped())
		{
			return;
		}

		if(!ec)
		{
			this->check_heartbeat();
		}
	});
}

template <typename socket_type>
inline void asio_connection<socket_type>::check_heartbeat()
{
	std::chrono::steady_clock::duration timestamp{this->heartbeat_timestamp_};
	std::chrono::steady_clock::time_point heartbeat_timestamp{timestamp};
	auto now = asio::steady_timer::clock_type::now();
	auto time_since_last_heartbeat =
		std::chrono::duration_cast<std::chrono::seconds>(now - heartbeat_timestamp);

	if(time_since_last_heartbeat > heartbeat_check_interval_)
	{
		this->stop(make_error_code(errc::host_unreachable));
	}
	else
	{
		await_heartbeat();
	}
}
} // namespace net
