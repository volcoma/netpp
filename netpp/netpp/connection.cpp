#include "connection.h"
#include <atomic>

namespace net
{
static connection::id_t get_next_id()
{
	static std::atomic<connection::id_t> ids{0};
	return ++ids;
}

connection::connection()
    : msg_builder_(std::make_unique<standard_builder>())
	, id(get_next_id())
{
}

} // namespace net
