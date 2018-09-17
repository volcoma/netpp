#pragma once
#include "basic_client.hpp"
#include "compatibility.hpp"
#include <asio/ssl.hpp>
namespace net
{
namespace tcp
{
namespace detail
{
inline bool verify_certificate(bool preverified, asio::ssl::verify_context& ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.

	// In this example we will simply print the certificate's subject name.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	log() << "Verifying " << subject_name << "\n";

	return preverified;
}
}

template <typename protocol_type>
class basic_ssl_client : public basic_client<protocol_type>
{
public:
	using base_type = basic_client<protocol_type>;
	using protocol = typename base_type::protocol;
	using weak_ptr = typename base_type::weak_ptr;
	using protocol_endpoint = typename base_type::protocol_endpoint;
	using protocol_acceptor = typename base_type::protocol_acceptor;
	using protocol_socket = typename base_type::protocol_socket;

	basic_ssl_client(asio::io_service& io_context, const protocol_endpoint& listen_endpoint,
					 const std::string& cert_file)
		: base_type(io_context, listen_endpoint)
		, context_(asio::ssl::context::sslv23)
	{
		context_.load_verify_file(cert_file);
	}

	void start()
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
			ssl_socket->set_verify_mode(asio::ssl::verify_peer);
			ssl_socket->set_verify_callback(detail::verify_certificate);

			// Start the asynchronous handshake operation.
			ssl_socket->async_handshake(asio::ssl::stream_base::client,
										[weak_this, ssl_socket](const error_code& ec) mutable {
											if(ec)
											{
												log() << "[NET] : handshake error: " << ec.message() << "\n";

												auto shared_this = weak_this.lock();
												if(!shared_this)
												{
													return;
												}

												// We need to close the socket used in the previous connection
												// attempt before starting a new one.
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

		this->async_connect(socket, std::move(on_connection_established));
	}

protected:
	asio::ssl::context context_;
};
}
} // namespace net
