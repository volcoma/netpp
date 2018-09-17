#include "connector.h"
#include <atomic>
namespace net
{
static connector::id_t get_next_id()
{
	static std::atomic<connector::id_t> ids{0};
	return ++ids;
}

connector::connector()
	: id(get_next_id())
{
}
} // namespace net
