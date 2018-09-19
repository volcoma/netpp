#pragma once
#include <netpp/connection.h>

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_service.hpp>
#include <asio/strand.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <deque>
#include <chrono>
#include <thread>

namespace net
{
namespace udp
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
// Once a message is available, it is sent to the client. The deadline for
// sending a complete message is 30 seconds. After the message is successfully
// sent, the output actor again waits for the output queue to become non-empty.

template <typename socket_type>
class async_connection : public connection, public std::enable_shared_from_this<async_connection<socket_type>>
{
public:
	async_connection(const std::shared_ptr<socket_type>& socket, asio::io_service& context);

	void start() override;
	void stop(const error_code& ec) override;

private:
	bool stopped() const;
	void send_msg(byte_buffer&& msg, data_channel channel) override;

	void start_read();
	void handle_read(const error_code& ec, std::size_t n);
	void await_output();
	void start_write();
	void handle_write(const error_code& ec);

	mutable std::mutex guard_;

    //deque to avoid elements invalidation when resizing
	std::deque<std::vector<byte_buffer>> output_queue_;

	asio::io_service::strand strand_;

	std::shared_ptr<socket_type> socket_;
	asio::steady_timer non_empty_output_queue_;

	std::atomic<bool> connected{false};
};

template <typename socket_type>
async_connection<socket_type>::async_connection(const std::shared_ptr<socket_type>& socket,
												asio::io_service& context)
	: strand_(context)
	, socket_(socket)
	, non_empty_output_queue_(context)
{
	socket_->lowest_layer().non_blocking(true);
	// The non_empty_output_queue_ steady_timer is set to the maximum time
	// point whenever the output queue is empty. This ensures that the output
	// actor stays asleep until a message is put into the queue.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::max());

	msg_builder_ = std::make_unique<single_buffer_builder>();
}

template <typename socket_type>
void async_connection<socket_type>::start()
{
	connected = true;
	start_read();
	await_output();
}
template <typename socket_type>
void async_connection<socket_type>::stop(const error_code& ec)
{
	{
		std::lock_guard<std::mutex> lock(guard_);
		non_empty_output_queue_.cancel();
	}

	if(connected.exchange(false))
	{
		for(const auto& callback : on_disconnect)
		{
			callback(id, ec ? ec : asio::error::make_error_code(asio::error::connection_aborted));
		}
	}
}
template <typename socket_type>
bool async_connection<socket_type>::stopped() const
{
	std::lock_guard<std::mutex> lock(guard_);
	return !socket_->lowest_layer().is_open();
}

template <typename socket_type>
void async_connection<socket_type>::send_msg(byte_buffer&& msg, data_channel channel)
{
    std::lock_guard<std::mutex> lock(guard_);
	auto buffers = msg_builder_->build(std::move(msg), channel);

	output_queue_.emplace_back(std::move(buffers));

	// Signal that the output queue contains messages. Modifying the expiry
	// will wake the output actor, if it is waiting on the timer.
	non_empty_output_queue_.expires_at(asio::steady_timer::time_point::min());
}

template <typename socket_type>
void async_connection<socket_type>::start_read()
{
	// Start an asynchronous operation to read a certain number of bytes.
    socket_->async_receive(asio::null_buffers(), asio::socket_base::message_peek,
					 strand_.wrap(std::bind(&async_connection::handle_read, this->shared_from_this(),
											std::placeholders::_1, std::placeholders::_2)));
}
template <typename socket_type>
void async_connection<socket_type>::handle_read(const error_code& ec, std::size_t size)
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
            if(!socket_->can_read())
            {
                std::this_thread::yield();
                break;
            }
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

            auto processed = 0;
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

template <typename socket_type>
void async_connection<socket_type>::await_output()
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
template <typename socket_type>
void async_connection<socket_type>::start_write()
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
	asio::async_write(*socket_, buffers,
					  strand_.wrap(std::bind(&async_connection::handle_write, this->shared_from_this(),
											 std::placeholders::_1)));
}
template <typename socket_type>
void async_connection<socket_type>::handle_write(const error_code& ec)
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
