#pragma once
#include <netpp/connector.h>
#include <memory>
#include <mutex>
namespace net
{

class messenger : public std::enable_shared_from_this<messenger>
{
public:
	using ptr = std::shared_ptr<messenger>;
	using weak_ptr = std::weak_ptr<messenger>;
    using on_msg_t = std::function<void(connection::id_t, const byte_buffer&)>;
	using on_connect_t = std::function<void(connection::id_t)>;
	using on_disconnect_t = std::function<void(connection::id_t, const error_code&)>;

	static ptr create();

	connector::id_t add_connector(const connector_ptr& connector, on_connect_t on_connect,
								  on_disconnect_t on_disconnect, on_msg_t on_msg);
	void send_msg(connection::id_t id, byte_buffer&& msg);

	void disconnect(connection::id_t id);
	void remove_connector(connector::id_t id);

	void stop();

	bool empty() const;

private:
	messenger() = default;

	struct user_info
	{
		on_msg_t on_msg;
		on_connect_t on_connect;
		on_disconnect_t on_disconnect;
	};
	using user_info_ptr = std::shared_ptr<user_info>;
	using user_info_weak_ptr = std::weak_ptr<user_info>;

	struct connection_info
	{
        std::shared_ptr<void> sentinel;
		connection_ptr connection;
	};
	void on_new_connection(connection_ptr& connection, const user_info_ptr& info);

	void on_disconnect(connection::id_t id, error_code ec, const user_info_ptr& info);
	void on_msg(connection::id_t id, const byte_buffer& msg, const user_info_ptr& info);

	mutable std::mutex guard_;
	std::map<connector::id_t, connector_ptr> connectors_;
	std::map<connection::id_t, connection_info> connections_;
};

void deinit_messengers();
net::messenger::ptr get_network();

} // namespace net
