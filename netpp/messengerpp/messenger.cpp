#include "messenger.h"

namespace net
{

std::vector<std::function<void()>>& get_deleters()
{
	static std::vector<std::function<void()>> deleters;
	return deleters;
}

std::mutex& get_messengers_mutex()
{
	static std::mutex s_mutex;
	return s_mutex;
}

void deinit_messengers()
{
	auto& mutex = get_messengers_mutex();
	std::unique_lock<std::mutex> lock(mutex);
	auto deleters = std::move(get_deleters());
	lock.unlock();
	for(auto& deleter : deleters)
	{
		deleter();
	}
}

} // namespace net
