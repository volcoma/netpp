#pragma once
#include "../config.h"
#include "ssl_parsing.hpp"
#include <asio/ssl.hpp>
namespace net
{
namespace tcp
{

class basic_ssl_entity
{
public:
	//-----------------------------------------------------------------------------
	/// Constructor of ssl entity with certificates.
	//-----------------------------------------------------------------------------
	basic_ssl_entity(const ssl_config& config);

protected:
	asio::ssl::context context_;
	ssl_config config_;
};

inline basic_ssl_entity::basic_ssl_entity(const ssl_config& config)
	: context_(utils::to_asio_method(config.method))
	, config_(config)
{
	context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
						 asio::ssl::context::single_dh_use);

	if(!config.cert_chain_file.empty())
	{
		context_.use_certificate_chain_file(config.cert_chain_file);
	}

	int mode = asio::ssl::verify_peer;
	if(config.require_peer_cert)
	{
		mode |= asio::ssl::verify_fail_if_no_peer_cert;
	}
	context_.set_verify_mode(mode);

	if(!config.cert_auth_file.empty())
	{
		context_.load_verify_file(config.cert_auth_file);
	}

	if(config.verify_callback)
	{
		context_.set_verify_callback([verify_callback = config.verify_callback](
			bool preverified, asio::ssl::verify_context& ctx) {

			ssl_certificate cert;
			preverified = utils::verify(preverified, ctx, cert);

			if(preverified)
			{
				preverified &= verify_callback(cert);
			}

			return preverified;
		});
	}

	// Setup password callback only if password was provided
	if(!config.private_key_password.empty())
	{
		// clang-format off
		context_.set_password_callback([password = config.private_key_password](
			std::size_t max_length, asio::ssl::context::password_purpose /*purpose*/) -> std::string
        {
            if(password.size() > max_length)
            {
                log() << "Provided password is too long.";
                return {};
            }
            return password;
		});
		// clang-format on
	}

	if(!config.private_key_file.empty())
	{
		context_.use_private_key_file(config.private_key_file, asio::ssl::context::pem);
	}

	// Use DH Parameters only if provided
	if(!config.dh_file.empty())
	{
		context_.use_tmp_dh_file(config.dh_file);
	}
}
}
} // namespace net
