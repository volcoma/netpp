#pragma once
#include "connector.h"
#include <mutex>
namespace net
{

class messenger
{
public:
	using on_msg_t = connector::on_msg_t;
	using on_connect_t = connector::on_connect_t;
	using on_disconnect_t = connector::on_disconnect_t;

	connector::id add_connector(const connector_ptr& connector, on_connect_t on_connect,
								on_disconnect_t on_disconnect, on_msg_t on_msg);
	void send_msg(connection::id id, byte_buffer&& msg);

    void disconnect(connection::id id);
    void remove_connector(connector::id id);
private:
	struct connector_info
	{
		connector_ptr connector;
		on_msg_t on_msg;
		on_connect_t on_connect;
		on_disconnect_t on_disconnect;
	};
    std::mutex guard_;
	std::map<connector::id, connector_info> connectors_;
	std::map<connection::id, connector::id> connections_;
};
} // namespace net
