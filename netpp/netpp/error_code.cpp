#include "error_code.h"

namespace net
{
namespace
{
const net_err_category net_category{};
}
const char* net::net_err_category::name() const noexcept
{
	return "network";
}

std::string net::net_err_category::message(int ev) const
{
	switch(static_cast<errc>(ev))
	{
		case errc::illegal_data_format:
			return "Illegal data format.";

		case errc::user_triggered_disconnect:
			return "User triggered disconnect.";

		default:
			return "(Unrecognized error)";
	}
}

std::error_code make_error_code(errc e)
{
	return {static_cast<int>(e), net_category};
}
}
