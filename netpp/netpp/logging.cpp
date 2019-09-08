#include "logging.h"
#include <mutex>

namespace net
{

net::logger& get_logger()
{
	static logger l;
	return l;
}

void set_logger(const logger& log)
{
	get_logger() = log;
}

log::~log()
{
	auto& func = get_logger();
	if(func)
	{
        static std::mutex mutex{};
        std::lock_guard<std::mutex> lock(mutex);
		func(str.str());
	}
}
}
