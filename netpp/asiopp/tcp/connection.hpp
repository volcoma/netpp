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
	tcp_connection(std::shared_ptr<socket_type> socket, const msg_builder::creator& builder_creator,
				   asio::io_service& context, std::chrono::seconds heartbeat);

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

private:
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

	/// heartbeat interval
	std::chrono::seconds heartbeat_check_interval_;
	/// a steady timer to tick for heartbeat checks
	asio::steady_timer heartbeat_check_timer_;
	/// a steady timer to issue a heartbeat reply
	asio::steady_timer heartbeat_reply_timer_;
	/// the last time a heartbeat was sent/recieved
	std::atomic<asio::steady_timer::duration> heartbeat_timestamp_{};
};

template <typename socket_type>
inline tcp_connection<socket_type>::tcp_connection(std::shared_ptr<socket_type> socket,
												   const msg_builder::creator& builder_creator,
												   asio::io_service& context, std::chrono::seconds heartbeat)
	: base_type(socket, builder_creator, context)
	, heartbeat_check_interval_(heartbeat)
	, heartbeat_check_timer_(context)
	, heartbeat_reply_timer_(context)

{
	heartbeat_check_timer_.expires_at(asio::steady_timer::time_point::max());
	heartbeat_reply_timer_.expires_at(asio::steady_timer::time_point::max());
}

template <typename socket_type>
inline void tcp_connection<socket_type>::start()
{
	base_type::start();

	if(heartbeat_check_interval_ > std::chrono::seconds::zero())
	{
		send_heartbeat();
		await_heartbeat();
	}
}
template <typename socket_type>
inline void tcp_connection<socket_type>::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(this->guard_);
		heartbeat_check_timer_.cancel();
		heartbeat_reply_timer_.cancel();
	}

	base_type::stop(ec);
}

template <typename socket_type>
inline void tcp_connection<socket_type>::start_read()
{
	// NOTE! Thread safety:
	// the builder should only be used for reads
	// which are already synchronized via the explicit 'strand'

	auto operation = this->builder->get_next_operation();
	auto& work_buffer = this->builder->get_work_buffer();
	auto offset = work_buffer.size();
	work_buffer.resize(offset + operation.bytes);

	// Here std::bind + shared_from_this is used because of the composite op async_*
	// We want it to operate on valid data until the handler is called.
	// Start an asynchronous operation to read a certain number of bytes.
	asio::async_read(*this->socket_, asio::buffer(work_buffer.data() + offset, operation.bytes),
					 asio::transfer_exactly(operation.bytes),
					 this->strand_.wrap(std::bind(&base_type::handle_read, this->shared_from_this(),
												  std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type>
inline void tcp_connection<socket_type>::handle_read(const error_code& ec, std::size_t size)
{
	if(this->stopped())
	{
		return;
	}
	if(!ec)
	{
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
			return;
		}
		catch(...)
		{
			this->stop(make_error_code(errc::data_corruption));
			return;
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
		start_read();
	}
	else
	{
		this->stop(ec);
	}
}

template <typename socket_type>
inline void tcp_connection<socket_type>::start_write()
{
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
	asio::async_write(*this->socket_, buffers,
					  this->strand_.wrap(std::bind(&base_type::handle_write, this->shared_from_this(),
												   std::placeholders::_1)));
}
template <typename socket_type>
inline void tcp_connection<socket_type>::handle_write(const error_code& ec)
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

template <typename socket_type>
inline void tcp_connection<socket_type>::schedule_heartbeat()
{
	update_heartbeat_timestamp();

	std::lock_guard<std::mutex> lock(this->guard_);

	using namespace std::chrono_literals;
	heartbeat_reply_timer_.expires_after(2s);
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
inline void tcp_connection<socket_type>::update_heartbeat_timestamp()
{
	this->heartbeat_timestamp_ = asio::steady_timer::clock_type::now().time_since_epoch();
}

template <typename socket_type>
inline void tcp_connection<socket_type>::send_heartbeat()
{
	// send heartbeat
	this->send_msg({}, 0);

	update_heartbeat_timestamp();
}

template <typename socket_type>
inline void tcp_connection<socket_type>::await_heartbeat()
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
inline void tcp_connection<socket_type>::check_heartbeat()
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
}
} // namespace net
