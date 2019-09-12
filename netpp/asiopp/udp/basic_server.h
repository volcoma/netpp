#pragma once
#include "server_connection.h"
#include <netpp/connector.h>

#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>

namespace net
{
namespace udp
{

using asio::ip::udp;

class basic_server : public connector, public std::enable_shared_from_this<basic_server>
{
public:
    using weak_ptr = std::weak_ptr<basic_server>;
    ~basic_server() override = default;
	//-----------------------------------------------------------------------------
	/// Constructor of client accepting a receive endpoint.
	//-----------------------------------------------------------------------------
    basic_server(asio::io_service& io_context, udp::endpoint endpoint,
                 std::chrono::seconds heartbeat = std::chrono::seconds{0});

	//-----------------------------------------------------------------------------
	/// Starts the receiver creating an udp socket and setting proper options
	/// depending on the type of the endpoint
	//-----------------------------------------------------------------------------
	void start() override;

    void on_recv_data(std::shared_ptr<udp::socket> socket,
                      std::size_t n);

    void listen(std::shared_ptr<udp::socket> socket);
protected:
	void restart();
	udp::endpoint endpoint_;
	asio::io_service& io_context_;
	asio::steady_timer reconnect_timer_;
    std::chrono::seconds heartbeat_;

    input_buffer input_buffer_;
    udp::endpoint remote_endpoint_;

    std::shared_ptr<asio::io_service::strand> strand_;
    std::map<udp::endpoint, std::shared_ptr<udp_server_connection>> connections_;
};
}
} // namespace net
