#pragma once
#include <asio/io_service.hpp>
#include <asio/ip/udp.hpp>

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
		: base_type(io_context, endpoint)
		, endpoint_(endpoint)
	{
	}
	template <class... Args>
	void shutdown(Args&&... /*ignore*/) noexcept
	{
	}

	template <class... Args>
	void close(Args&&... /*ignore*/) noexcept
	{
	}

	template <class ConstBufferSequence, class WriteHandler>
	void async_write_some(const ConstBufferSequence& buffer, WriteHandler&& handler)
	{
        //auto endpoint = this->local_endpoint();
		//if(can_send)
		{
			this->async_send_to(buffer, endpoint_, std::forward<WriteHandler>(handler));
		}
	}

	template <class MutableBufferSequence, class ReadHandler>
	void async_read_some(const MutableBufferSequence& buffer, ReadHandler&& handler)
	{
        //auto endpoint = this->local_endpoint();

		//if(can_recieve)
		{
			this->async_receive_from(buffer, endpoint_, std::forward<ReadHandler>(handler));
		}
	}

private:
	endpoint_type endpoint_;
};

} // namespace net
