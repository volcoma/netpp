#pragma once

#include <system_error>
namespace net
{
using error_code = std::error_code;
//----------------------------------------------------------------------
enum class errc : int
{
	// no 0
	illegal_data_format = 1, // Illegal message data format
	user_triggered_disconnect
};
std::error_code make_error_code(errc);

struct net_err_category : std::error_category
{
	const char* name() const noexcept override;
	std::string message(int ev) const override;
};

} // namespace net

namespace std
{
template <>
struct is_error_code_enum<net::errc> : true_type
{
};
}
