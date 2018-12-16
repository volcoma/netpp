#pragma once

#include <system_error>
namespace net
{
using error_code = std::error_code;
//----------------------------------------------------------------------
enum class errc : int
{
	// no 0
	data_corruption = 1, // Data corruption or unknown data format
	user_triggered_disconnect = 2,
	host_unreachable = 3
};
std::error_code make_error_code(errc);

struct net_err_category : std::error_category
{
	const char* name() const noexcept override;
	std::string message(int ev) const override;
};

bool is_data_corruption_error(const error_code& ec);

} // namespace net

namespace std
{
template <>
struct is_error_code_enum<net::errc> : true_type
{
};
}
