#pragma once
#include <netpp/connector.h>

#include <asio/ip/udp.hpp>

namespace net
{
namespace udp
{
using asio::ip::udp;

class basic_sender : public connector, public std::enable_shared_from_this<basic_sender>
{
public:
    using weak_ptr = std::weak_ptr<basic_sender>;
    ~basic_sender() override = default;
    //-----------------------------------------------------------------------------
    /// Constructor of client accepting a send endpoint (uni/multi/broad cast)
    //-----------------------------------------------------------------------------
	basic_sender(asio::io_service& io_context, udp::endpoint endpoint);

    //-----------------------------------------------------------------------------
    /// Starts the receiver creating an udp socket and setting proper options
    /// depending on the type of the endpoint
    //-----------------------------------------------------------------------------
	void start() override;

protected:
    /// (uni/multi/broad cast) endpoint
	udp::endpoint endpoint_;
	asio::io_service& io_context_;
};
}
} // namespace net
