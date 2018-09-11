#pragma once
#include "connector.h"
#include <asio/ip/udp.hpp>

namespace net
{
using asio::ip::udp;
////----------------------------------------------------------------------

class udp_connection : public connection
{
public:
	udp_connection(asio::io_context& io_context, udp::endpoint endpoint);

private:
	void send_msg(byte_buffer&& msg) override;

	udp::socket socket_;
    udp::endpoint endpoint_;
};
} // namespace net
