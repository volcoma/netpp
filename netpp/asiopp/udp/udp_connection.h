#pragma once
#include <asio/ip/udp.hpp>
#include <netpp/connection.h>

namespace net
{
using asio::ip::udp;
////----------------------------------------------------------------------

class udp_connection : public connection
{
public:
	udp_connection(asio::io_context& io_context, udp::endpoint endpoint);

private:
	void send_msg(byte_buffer&& msg, data_channel channel) override;

	udp::socket socket_;
	udp::endpoint endpoint_;
};
} // namespace net
