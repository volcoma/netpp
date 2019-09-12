#include "basic_server.h"
#include "connection.h"
#include <asio/ip/multicast.hpp>

namespace net
{
namespace udp
{

basic_server::basic_server(asio::io_service& io_context, udp::endpoint endpoint,
                           std::chrono::seconds heartbeat)
	: endpoint_(std::move(endpoint))
	, io_context_(io_context)
	, reconnect_timer_(io_context)
    , heartbeat_(heartbeat)
    , strand_(std::make_shared<asio::io_service::strand>(io_context_))
{
	reconnect_timer_.expires_at(asio::steady_timer::time_point::max());
}

void basic_server::start()
{
    connections_.clear();

	auto socket = std::make_shared<udp::socket>(io_context_);
    udp::endpoint socket_endpoint;

    if (endpoint_.address().is_v4())
    {
        socket_endpoint = udp::endpoint(udp::v4(), endpoint_.port());
    }
    else if (endpoint_.address().is_v6())
    {
        socket_endpoint = udp::endpoint(udp::v6(), endpoint_.port());
    }

	asio::error_code ec;
    if(socket->open(socket_endpoint.protocol(), ec))
	{
		log() << "[Error] datagram_socket::open : " << ec.message();
	}
	if(socket->set_option(asio::socket_base::reuse_address(true), ec))
	{
		log() << "[Error] datagram_socket::reuse_address : " << ec.message();
	}

    if(socket->bind(socket_endpoint, ec))
	{
		log() << "[Error] datagram_socket::bind : " << ec.message();
	}
	if(endpoint_.address().is_multicast())
	{
		if(socket->set_option(asio::ip::multicast::join_group(endpoint_.address()), ec))
		{
			log() << "[Error] datagram_socket::multicast::join_group : " << ec.message();
		}
	}

    listen(socket);
}

void basic_server::on_recv_data(std::shared_ptr<udp::socket> socket,
                                std::size_t size)
{
    auto it = connections_.find(remote_endpoint_);
    if (it == std::end(connections_))
    {
        auto session = std::make_shared<udp_server_connection>(socket, create_builder, io_context_, heartbeat_);
        session->set_options(remote_endpoint_, strand_);

        auto result = connections_.emplace(remote_endpoint_, session);
        if (!result.second)
        {
            return;
        }

        if (on_connection_ready)
        {
            on_connection_ready(session);
        }

        auto weak_this = weak_ptr(shared_from_this());
        session->on_disconnect.emplace_back([weak_this, remote_endpoint = remote_endpoint_](connection::id_t, const error_code&) {
            auto shared_this = weak_this.lock();
            if(!shared_this)
            {
                return;
            }

            shared_this->io_context_.post(shared_this->strand_->wrap([shared_this, remote_endpoint]()
            {
                auto sz = shared_this->connections_.size();
                shared_this->connections_.erase(remote_endpoint);
                auto szafter = shared_this->connections_.size();

                net::log() << "sz before : " << sz << ", sz after : " << szafter;
            }));

        });

        it = result.first;
    }
    auto total_size = input_buffer_.offset + size;

    auto processed = it->second->process_read(input_buffer_.buffer.data(), total_size);

    auto left = total_size - processed;

    if(processed != 0)
    {
        if(processed < left)
        {
            int a = 0;
            a++;
        }

        assert(processed >= left);
        std::memcpy(input_buffer_.buffer.data(), input_buffer_.buffer.data() + processed, left);
    }

    input_buffer_.offset = left;

    listen(socket);
}

void basic_server::listen(std::shared_ptr<udp::socket> socket)
{

    auto& service = socket->get_io_service();
    service.post(strand_->wrap([this, shared_this = shared_from_this(), socket]() {

        // Start an asynchronous operation to read a certain number of bytes.
        // Here std::bind + shared_from_this is used because of the composite op async_*
        // We want it to operate on valid data until the handler is called.
        socket->async_receive_from(
            asio::buffer(input_buffer_.buffer.data() + input_buffer_.offset,
            input_buffer_.buffer.size() - input_buffer_.offset),
            remote_endpoint_,
            strand_->wrap([shared_this, socket](const error_code& ec, std::size_t size) mutable
            {
                if(ec)
                {
                    log() << "Recv error: " << ec.message();

                     // Start accepting new connections
                     shared_this->restart();
                }
                else
                {
                    shared_this->on_recv_data(socket, size);
                }
            }));
    }));
}

void basic_server::restart()
{
	using namespace std::chrono_literals;
	reconnect_timer_.expires_from_now(1s);
    reconnect_timer_.async_wait(strand_->wrap(std::bind(&basic_server::start, shared_from_this())));
}
}
} // namespace net
