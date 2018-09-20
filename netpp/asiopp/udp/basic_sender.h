#pragma once
#include "connection.h"

#include <netpp/connector.h>

#include <asio/ip/multicast.hpp>

namespace net
{
namespace udp
{
using asio::ip::udp;

class basic_sender : public connector, public std::enable_shared_from_this<basic_sender>
{
public:
	using weak_ptr = std::weak_ptr<basic_sender>;

	basic_sender(asio::io_service& io_context, udp::endpoint endpoint);

	void start() override;

protected:
	udp::endpoint endpoint_;
	asio::io_service& io_context_;
};
}
} // namespace net
