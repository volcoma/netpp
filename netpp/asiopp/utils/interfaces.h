#pragma once

#include <map>
#include <vector>

#include "../address.h"

namespace net
{
namespace utils
{
namespace interfaces
{

struct iface_data
{
    std::string mac {};
    std::vector<ip::address> addresses {};
};

std::map<std::string, iface_data> get_local_interfaces();

}
}
}
