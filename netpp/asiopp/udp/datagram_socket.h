#pragma once
#include <asio/io_service.hpp>
#include <asio/ip/multicast.hpp>
#include <asio/ip/udp.hpp>
#include <netpp/logging.h>
namespace net
{

////----------------------------------------------------------------------
template <typename protocol, bool can_send, bool can_recieve>
struct datagram_socket : public asio::basic_datagram_socket<protocol>
{
public:
    using base_type = asio::basic_datagram_socket<protocol>;
    using base_type::base_type;
    using endpoint_type = typename asio::basic_datagram_socket<protocol>::endpoint_type;
    datagram_socket(asio::io_service& io_context, const endpoint_type& endpoint)
        : base_type(io_context)
        , endpoint_(endpoint)
    {
        asio::error_code ec;
        if(this->open(endpoint_.protocol(), ec))
        {
            log() << "[Error] datagram_socket::open : " << ec.message() << "\n";
        }
        if(this->set_option(asio::socket_base::reuse_address(true), ec))
        {
            log() << "[Error] datagram_socket::reuse_address : " << ec.message() << "\n";
        }

        if(can_recieve)
        {
            if(this->bind(endpoint_, ec))
            {
                log() << "[Error] datagram_socket::bind : " << ec.message() << "\n";
            }
        }

        if(can_send)
        {
            if(this->set_option(asio::socket_base::broadcast(true), ec))
            {
                log() << "[Error] datagram_socket::broadcast : " << ec.message() << "\n";
            }
            // sock is now open. Any failure in this scope will require a close()
            if(this->set_option(asio::ip::multicast::enable_loopback(false), ec))
            {
                log() << "[Error] datagram_socket::multicast::enable_loopback : " << ec.message() << "\n";
            }
        }



        if(can_recieve)
        {
            if(this->set_option(asio::ip::multicast::join_group(endpoint_.address()), ec))
            {
                log() << "[Error] datagram_socket::multicast::join_group : " << ec.message() << "\n";
            }
        }
    }
    template <class... Args>
    void shutdown(Args&&... /*ignore*/) noexcept
    {
    }

    template <class... Args>
    void close(Args&&... /*ignore*/) noexcept
    {
    }

    bool can_write() const
    {
        return can_send;
    }

    bool can_read() const
    {
        return can_recieve;
    }

    template <class ConstBufferSequence, class WriteHandler>
    void async_write_some(const ConstBufferSequence& buffer, WriteHandler&& handler)
    {
        if(can_send)
        {
            this->async_send_to(buffer, endpoint_, std::forward<WriteHandler>(handler));
        }
    }

    template <class MutableBufferSequence, class ReadHandler>
    void async_read_some(const MutableBufferSequence& buffer, ReadHandler&& handler)
    {
        if(can_recieve)
        {
            this->async_receive_from(buffer, endpoint_, std::forward<ReadHandler>(handler));
        }
    }

private:
    endpoint_type endpoint_;
};

} // namespace net
