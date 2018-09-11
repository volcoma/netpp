#pragma once

#include <asio/error.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace net
{
using namespace asio;
using byte_buffer = std::string; // std::vector<uint8_t>;
//----------------------------------------------------------------------
class connection;
using connection_ptr = std::shared_ptr<connection>;

class connection
{
public:
	using id = uint64_t;
	connection();
	virtual ~connection() = default;

	virtual void send_msg(byte_buffer&&) = 0;
	virtual void start() = 0;
	virtual void stop(const asio::error_code& ec) = 0;

	const id& get_id() const;

    using on_connect = std::function<void(connection_ptr)>;
    using on_disconnect = std::function<void(connection_ptr, const asio::error_code&)>;
    using on_msg = std::function<void(connection_ptr, const byte_buffer&)>;

    on_connect on_connect_;
    on_disconnect on_disconnect_;
    on_msg on_msg_;
private:
	id id_;
};

//----------------------------------------------------------------------

class channel
{
public:
	using connection_ptr = std::shared_ptr<connection>;
	void join(const connection_ptr& conn);
	void leave(const connection_ptr& conn);
	void stop(const asio::error_code& ec);
	void send_msg(connection::id id, byte_buffer&& msg);

private:
	std::map<connection::id, connection_ptr> connections_;
};

} // namespace net
