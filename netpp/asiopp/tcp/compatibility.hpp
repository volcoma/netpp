#pragma once
#include <asio/ssl.hpp>
#include <asio/version.hpp>

namespace net
{
namespace tcp
{
namespace compatibility
{

template<typename protocol_socket>
auto make_socket(asio::io_service& io_context)
{
#if ASIO_VERSION < 101200
    struct old_version_workaround : protocol_socket
    {
        using protocol_socket::protocol_socket;
        old_version_workaround(old_version_workaround& sock)
            : protocol_socket(std::move(static_cast<protocol_socket&>(sock)))
        {}
    };
    using protocol_socket_type = old_version_workaround;
#else
    using protocol_socket_type = protocol_socket;
#endif

    return std::make_shared<protocol_socket_type>(io_context);
}
template<typename protocol_socket>
auto make_ssl_socket(protocol_socket&& lower_layer, asio::ssl::context& context)
{
#if ASIO_VERSION < 101200
    auto socket = std::move(lower_layer);
    return std::make_shared<asio::ssl::stream<protocol_socket>>(socket, context);
#else
    return std::make_shared<asio::ssl::stream<protocol_socket>>(std::move(lower_layer), context);
#endif

}
}
}
} // namespace net
