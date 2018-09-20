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
	static std::mutex guard;
	std::lock_guard<std::mutex> lock(guard);
	auto& func = get_logger();
	if(func)
	{
		func(str.str());
	}
}
}
