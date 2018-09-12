#pragma once
#include <netpp/connector.h>

#include <asio/io_context.hpp>

namespace net
{
using namespace asio;

class asio_connector : public connector
{
public:
	asio_connector(asio::io_context& context);

protected:
	asio::io_context& io_context;
};

} // namespace net
