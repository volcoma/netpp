#pragma once
#include <netpp/connection.h>

#include <asio/basic_stream_socket.hpp>
#include <asio/buffer.hpp>
#include <asio/error.hpp>
#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/read.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>
#include <asio/write.hpp>
#include <chrono>
#include <deque>
#include <thread>

namespace net
{
namespace udp
{

using asio::ip::udp;

class async_connection : public connection, public std::enable_shared_from_this<async_connection>
{
public:
	async_connection(std::shared_ptr<udp::socket> socket, udp::endpoint endpoint, asio::io_service& context,
					 bool can_read, bool can_write);

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

	// deque to avoid elements invalidation when resizing
	std::deque<std::vector<byte_buffer>> output_queue_;

	asio::io_service::strand strand_;
	udp::endpoint endpoint_;
	udp::endpoint remote_endpoint_;
	std::shared_ptr<udp::socket> socket_;
	asio::steady_timer non_empty_output_queue_;

	std::atomic<bool> connected_{false};
	bool can_read_ = false;
	bool can_write_ = false;
};
}
} // namespace net
