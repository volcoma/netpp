#include "tcp_server.h"

namespace net
{

void tcp_server::start()
{
	using socket_type = tcp::socket;
	auto socket = std::make_shared<tcp::socket>(io_context);

	auto weak_this = weak_ptr(shared_from_this());
	auto on_connection_established = [weak_this, socket]() {
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}
		shared_this->on_handshake_complete(socket);
	};

	async_accept(socket, std::move(on_connection_established));
}

} // namespace net
