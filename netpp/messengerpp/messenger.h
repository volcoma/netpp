#ifndef MESSENGER_H
#define MESSENGER_H

#include <memory>
#include <mutex>
#include <map>
#include <netpp/connector.h>

namespace net
{

template <typename T, typename OArchive, typename IArchive>
struct serializer
{
	static_assert(sizeof(T) == 0,
				  R"(Specialization required!
    Please specialize(fully or partial) this type for the message type
    and (output archive)/(input archive) your messenger is working with.
    example:
    template <typename T>
    struct serializer<T, std::stringstream, std::stringstream>
    {
        static byte_buffer to_buffer(const T& msg)
        {
            std::stringstream serializer;
            serializer << msg;
            auto str = serializer.str();
            return byte_buffer{std::begin(str), std::end(str)};
        }
        static T from_buffer(byte_buffer&& buffer)
        {
            std::stringstream deserializer;
            deserializer << std::string{std::begin(buffer), std::end(buffer)};
            T msg;
            deserializer >> msg;
            return msg;
        }
    };)");

	static byte_buffer to_buffer(const T&);
	static T from_buffer(byte_buffer&&);
};

template <typename T, typename OArchive, typename IArchive>
class messenger : public std::enable_shared_from_this<messenger<T, OArchive, IArchive>>
{
public:
	using ptr = std::shared_ptr<messenger>;
	using weak_ptr = std::weak_ptr<messenger>;

	using msg_t = T;
	using serializer_t = serializer<T, OArchive, IArchive>;

	using on_connect_t = std::function<void(connection::id_t)>;
	using on_disconnect_t = std::function<void(connection::id_t, const error_code&)>;
	using on_msg_t = std::function<void(connection::id_t, msg_t)>;
	//-----------------------------------------------------------------------------
	/// Creates a messenger
	//-----------------------------------------------------------------------------
	static ptr create();

	//-----------------------------------------------------------------------------
	/// Removes all connnectors and disconnects all connections.
	//-----------------------------------------------------------------------------
	void remove_all();

	//-----------------------------------------------------------------------------
	/// Adds a connector to the messenger with callbacks which will be called
	/// on the same thread that triggered this function. Thread safe.
	/// 'connector' - a shared_ptr to the connector.
	/// 'on_connect' - callback to be triggered when a new connection occur.
	/// One connector can initiate multiple connections.
	/// 'on_disconnect' - callback to be triggered when a connection is disconnected.
	/// 'on_msg' - callback to be triggered when a new message is received from
	/// a specific connection.
	/// 'on_request' - callback to be triggered when a new request that expects a
	/// response is received from a specific connection.
	//-----------------------------------------------------------------------------
	connector::id_t add_connector(connector_ptr connector, on_connect_t on_connect,
								  on_disconnect_t on_disconnect, on_msg_t on_msg);

	//-----------------------------------------------------------------------------
	/// Removes a connector from messenger. Thread safe.
	/// 'id' - the id of the connector to be removed.
	//-----------------------------------------------------------------------------
	void remove_connector(connector::id_t id);

	//-----------------------------------------------------------------------------
	/// Sends a message to the specified connection. Thread safe.
	/// 'id' - the desired receiver of the message.
	/// 'msg' - the user defined message.
	//-----------------------------------------------------------------------------
	void send_msg(connection::id_t id, msg_t&& msg);

	//-----------------------------------------------------------------------------
	/// Disconnects the specified connection. Thread safe.
	/// 'id' - the connection to be disconnected.
	//-----------------------------------------------------------------------------
	void disconnect(connection::id_t id,
					const error_code& err = make_error_code(errc::user_triggered_disconnect));

	bool empty() const;

private:
	messenger() = default;

	struct user_info
	{
		on_connect_t on_connect{};
		on_disconnect_t on_disconnect{};
		on_msg_t on_msg{};

		connector::id_t connector_id{};
	};
	using user_info_ptr = std::shared_ptr<user_info>;

	struct connection_info
	{
		std::shared_ptr<void> sentinel;
		connection_ptr connection;
	};
	void on_new_connection(connection_ptr& connection, const user_info_ptr& info);
	void on_connect(connection::id_t id, connection_info&& conn_info, const user_info_ptr& info);
	void on_disconnect(connection::id_t id, error_code ec, const user_info_ptr& info);
	void on_raw_msg(connection::id_t id, byte_buffer& raw_msg, data_channel channel,
					const user_info_ptr& info);

	void on_msg(connection::id_t id, msg_t& msg, const user_info_ptr& info);
	void send(connection::id_t id, msg_t& msg, data_channel channel);

	/// lock for container synchronization
	mutable std::mutex guard_;
	std::map<connector::id_t, connector_ptr> connectors_;
	std::map<connection::id_t, connection_info> connections_;
};

std::vector<std::function<void()>>& get_deleters();
std::mutex& get_messengers_mutex();
void deinit_messengers();
template <typename T, typename OArchive, typename IArchive>
typename net::messenger<T, OArchive, IArchive>::ptr get_messenger();

} // namespace net

#endif

#include "messenger.hpp"
