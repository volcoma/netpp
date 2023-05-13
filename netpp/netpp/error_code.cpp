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
        case errc::data_corruption:
            return "Data corruption or unknown data format.";

        case errc::user_triggered_disconnect:
            return "User triggered disconnect.";

        case errc::host_unreachable:
            return "Host is unreachable.";

        default:
            return "(Unrecognized error)";
    }
}

std::error_code make_error_code(errc e)
{
    return {static_cast<int>(e), net_category};
}

bool is_data_corruption_error(const error_code& ec)
{
    return ec == make_error_code(errc::data_corruption);
}

}
