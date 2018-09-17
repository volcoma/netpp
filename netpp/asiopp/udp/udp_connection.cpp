#include <utility>

#include "udp_connection.h"

namespace net
{

udp_connection::udp_connection(asio::io_service& io_context, udp::endpoint endpoint)
	: socket_(io_context)
	, endpoint_(std::move(endpoint))
{
	socket_.set_option(udp::socket::broadcast(true));
}

void udp_connection::send_msg(byte_buffer&& msg, data_channel channel)
{
	asio::error_code ignored_ec;
	socket_.send_to(asio::buffer(msg), endpoint_, 0, ignored_ec);
}

} // namespace net
