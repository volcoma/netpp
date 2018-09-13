#include "connector.h"
namespace net
{

asio_connector::asio_connector(asio::io_context& context)
	: io_context_(context)
    , strand_(context)
{
}

} // namespace net
