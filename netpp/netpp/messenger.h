#pragma once
#include "connector.h"
#include <memory>
#include <mutex>
namespace net
{

class messenger : public std::enable_shared_from_this<messenger>
{
public:
	using ptr = std::shared_ptr<messenger>;
	using weak_ptr = std::weak_ptr<messenger>;
	using on_msg_t = connector::on_msg_t;
	using on_connect_t = connector::on_connect_t;
	using on_disconnect_t = connector::on_disconnect_t;

	static ptr create();

	connector::id_t add_connector(const connector_ptr& connector, on_connect_t on_connect,
								  on_disconnect_t on_disconnect, on_msg_t on_msg);
	void send_msg(connection::id_t id, byte_buffer&& msg);

	void disconnect(connection::id_t id);
	void remove_connector(connector::id_t id);

	size_t get_connections_count() const;
	void stop();

private:
	messenger() = default;
	void on_connect(connector::id_t connector_id, connection::id_t id);
	void on_disconnect(connector::id_t connector_id, connection::id_t id, error_code ec);
	void on_msg(connector::id_t connector_id, connection::id_t id, const byte_buffer& msg);

	struct connector_info
	{
		connector_ptr connector;
		on_msg_t on_msg;
		on_connect_t on_connect;
		on_disconnect_t on_disconnect;
	};
	mutable std::mutex guard_;
	std::map<connector::id_t, std::shared_ptr<connector_info>> connectors_;
	std::map<connection::id_t, connector::id_t> connections_;
};

void deinit_messengers();
net::messenger::ptr get_network();

} // namespace net
