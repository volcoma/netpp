#pragma once
#include "../service.h"
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
};

inline basic_ssl_entity::basic_ssl_entity(const ssl_config& config)
	: context_(asio::ssl::context::tlsv12)
{
	context_.set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
						 asio::ssl::context::single_dh_use);


	if(!config.cert_chain_file.empty())
	{
		context_.use_certificate_chain_file(config.cert_chain_file);
	}

    if(!config.cert_auth_file.empty())
	{
		context_.load_verify_file(config.cert_auth_file);
		context_.set_verify_mode(asio::ssl::verify_peer);
		context_.set_verify_callback([](bool preverified, asio::ssl::verify_context& ctx) {
			// The verify callback can be used to check whether the certificate that is
			// being presented is valid for the peer. For example, RFC 2818 describes
			// the steps involved in doing this for HTTPS. Consult the OpenSSL
			// documentation for more details. Note that the callback is called once
			// for each certificate in the certificate chain, starting from the root
			// certificate authority.

			// In this example we will simply print the certificate's subject name.
			std::array<char, 256> subject_name;
			X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
			X509_NAME_oneline(X509_get_subject_name(cert), subject_name.data(), subject_name.size());
			log() << "Verifying " << subject_name.data();

			OpenSSL_add_all_digests();
			const EVP_MD* digest = EVP_get_digestbyname("sha256");
			if(digest)
			{
				std::vector<uint8_t> md(EVP_MAX_MD_SIZE, 0);
				unsigned int n{};
				X509_digest(cert, digest, md.data(), &n);
				md.resize(n);
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
