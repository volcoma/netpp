#include "server_connection.h"

namespace net
{
namespace udp
{

void udp_server_connection::set_strand(std::shared_ptr<asio::io_service::strand> strand)
{
	strand_ = std::move(strand);
}

void udp_server_connection::stop_socket()
{
}

void udp_server_connection::start_read()
{
}
}
} // namespace net
