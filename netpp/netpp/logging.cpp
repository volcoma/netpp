#include "logging.h"
#include <mutex>
namespace net
{
namespace
{
std::mutex logger_guard{};
}
net::logger& get_logger()
{
    static logger l;
    return l;
}

void set_logger(const logger& log)
{
    std::lock_guard<std::mutex> lock(logger_guard);
    auto& func = get_logger();

    if(!func)
    {
        func = log;
    }
}

log::~log()
{
    std::unique_lock<std::mutex> lock(logger_guard);
    auto& func = get_logger();
    if(func)
    {
        lock.unlock();

        static std::mutex guard;
        std::lock_guard<std::mutex> lock(guard);
        func(str.str());
    }
}
}
