#pragma once
#include "basic_server.hpp"
#include "compatibility.hpp"

#include <asio/ssl.hpp>
namespace net
{
namespace tcp
{
template <typename protocol_type>
class basic_ssl_server : public basic_server<protocol_type>
{
public:
	//-----------------------------------------------------------------------------
	/// Aliases.
	//-----------------------------------------------------------------------------
	using base_type = basic_server<protocol_type>;
	using protocol = typename base_type::protocol;
	using weak_ptr = typename base_type::weak_ptr;
	using protocol_endpoint = typename base_type::protocol_endpoint;
	using protocol_acceptor = typename base_type::protocol_acceptor;
	using protocol_socket = typename base_type::protocol_socket;

	//-----------------------------------------------------------------------------
	/// Constructor of ssl server accepting a listen endpoint and certificates.
	//-----------------------------------------------------------------------------
	basic_ssl_server(asio::io_service& io_context, const protocol_endpoint& listen_endpoint,
					 const std::string& cert_chain_file, const std::string& private_key_file,
					 const std::string& dh_file);

	//-----------------------------------------------------------------------------
	/// Starts the server attempting to accept incomming connections.
	/// After a successful connect, a ssl handshake is performed to validate
	/// certificates and keys
	//-----------------------------------------------------------------------------
	void start() override;

protected:
	//-----------------------------------------------------------------------------
	/// Gets the password required.
	//-----------------------------------------------------------------------------
	std::string get_private_key_password() const;
	asio::ssl::context context_;
	std::string password_ = "test";
};

template <typename protocol_type>
inline basic_ssl_server<protocol_type>::basic_ssl_server(asio::io_service& io_context,
														 const protocol_endpoint& listen_endpoint,
														 const std::string& cert_chain_file,
														 const std::string& private_key_file,
														 const std::string& dh_file)
	: base_type(io_context, listen_endpoint)
	, context_(asio::ssl::context::sslv23)
{
	context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
						 asio::ssl::context::single_dh_use);

	context_.set_password_callback(std::bind(&basic_ssl_server::get_private_key_password, this));
	context_.use_certificate_chain_file(cert_chain_file);
	context_.use_private_key_file(private_key_file, asio::ssl::context::pem);
	context_.use_tmp_dh_file(dh_file);
}

template <typename protocol_type>
inline void basic_ssl_server<protocol_type>::start()
{
	auto socket = compatibility::make_socket<protocol_socket>(this->io_context_);

	auto weak_this = weak_ptr(this->shared_from_this());
	auto on_connection_established = [weak_this, this, socket = socket.get()]() mutable
	{
		auto shared_this = weak_this.lock();
		if(!shared_this)
		{
			return;
		}
		auto ssl_socket = compatibility::make_ssl_socket(std::move(*socket), context_);

		// Start the asynchronous handshake operation.
		ssl_socket->async_handshake(asio::ssl::stream_base::server,
									[weak_this, ssl_socket](const error_code& ec) mutable {
										if(ec)
										{
											log() << "handshake error: " << ec.message();

											auto shared_this = weak_this.lock();
											if(!shared_this)
											{
												return;
											}

											// We need to close the socket used in the previous connection
											// attempt
											// before starting a new one.
											ssl_socket.reset();
											// Try again.
											shared_this->start();
										}

										// Otherwise we have successfully established a connection.
										else
										{
											auto shared_this = weak_this.lock();
											if(!shared_this)
											{
												return;
											}
											shared_this->on_handshake_complete(ssl_socket);
										}
									});
	};

	this->async_accept(socket, std::move(on_connection_established));
}

template <typename protocol_type>
inline std::string basic_ssl_server<protocol_type>::get_private_key_password() const
{
	return password_;
}
}
} // namespace net
