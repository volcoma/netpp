#pragma once
#include "../config.h"
#include <asio/ssl.hpp>
#include <netpp/logging.h>
#include <sstream>

namespace net
{

namespace utils
{
//----------------------------------------------------------------------
inline int certversion(X509* x509)
{
	return static_cast<int>(X509_get_version(x509));
}
//----------------------------------------------------------------------
inline std::string _asn1int(ASN1_INTEGER* bs)
{
	static const char hexbytes[] = "0123456789ABCDEF";
	std::stringstream ashex;
	for(int i = 0; i < bs->length; i++)
	{
		ashex << hexbytes[(bs->data[i] & 0xf0) >> 4];
		ashex << hexbytes[(bs->data[i] & 0x0f) >> 0];
	}
	return ashex.str();
}
//----------------------------------------------------------------------
// inline std::string _asn1string(ASN1_STRING* d)
//{
//    std::string asn1_string;
//    if(ASN1_STRING_type(d) != V_ASN1_UTF8STRING)
//    {
//        unsigned char* utf8 = nullptr;
//        auto length = static_cast<size_t>(ASN1_STRING_to_UTF8(&utf8, d));
//        if(length > 0)
//        {
//            asn1_string = std::string(reinterpret_cast<char*>(utf8), length);
//            OPENSSL_free(utf8);
//        }
//    }
//    else
//    {
//        auto length = static_cast<size_t>(ASN1_STRING_length(d));
//        if(length > 0)
//        {
//            asn1_string = std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(d)), length);
//        }
//    }
//    return asn1_string;
//}
//----------------------------------------------------------------------
inline std::string serial(X509* x509)
{
	return _asn1int(X509_get_serialNumber(x509));
}

//----------------------------------------------------------------------
// inline std::string _subject_as_line(X509_NAME* subj_or_issuer)
//{
//    BIO* bio_out = BIO_new(BIO_s_mem());
//    X509_NAME_print(bio_out, subj_or_issuer, 0);
//    BUF_MEM* bio_buf = nullptr;
//    BIO_get_mem_ptr(bio_out, &bio_buf);
//    auto issuer = std::string(bio_buf->data, bio_buf->length);
//    BIO_free(bio_out);
//    return issuer;
//}
////----------------------------------------------------------------------
// inline std::map<std::string, std::string> _subject_as_map(X509_NAME* subj_or_issuer)
//{
//    std::map<std::string, std::string> m;
//    for(int i = 0; i < X509_NAME_entry_count(subj_or_issuer); i++)
//    {
//        X509_NAME_ENTRY* e = X509_NAME_get_entry(subj_or_issuer, i);
//        ASN1_STRING* d = X509_NAME_ENTRY_get_data(e);
//        ASN1_OBJECT* o = X509_NAME_ENTRY_get_object(e);
//        const char* key_name = OBJ_nid2sn(OBJ_obj2nid(o));
//        m[key_name] = _asn1string(d);
//    }
//    return m;
//}
//----------------------------------------------------------------------
// inline std::string issuer_one_line(X509* x509)
//{
//    return _subject_as_line(X509_get_issuer_name(x509));
//}
////----------------------------------------------------------------------
// inline std::string subject_one_line(X509* x509)
//{
//    return _subject_as_line(X509_get_subject_name(x509));
//}
////----------------------------------------------------------------------
// inline std::map<std::string, std::string> subject(X509* x509)
//{
//    return _subject_as_map(X509_get_subject_name(x509));
//}
////----------------------------------------------------------------------
// inline std::map<std::string, std::string> issuer(X509* x509)
//{
//    return _subject_as_map(X509_get_issuer_name(x509));
//}

//----------------------------------------------------------------------
inline int public_key_size(X509* x509)
{
	EVP_PKEY* pkey = X509_get_pubkey(x509);
	int keysize = EVP_PKEY_bits(pkey);
	EVP_PKEY_free(pkey);
	return keysize;
}

inline void log_cert(const ssl_certificate& cert)
{
	log() << "-------------------";
	log() << "Version: " << cert.version;
	log() << "Serial: " << cert.serial_number;
	log() << "Issuer: " << cert.issuer;
	for(const auto& kvp : cert.issuer_properties)
	{
		log() << " - " << kvp.first << " : " << kvp.second;
	}
	log() << "Subject: " << cert.subject;
	for(const auto& kvp : cert.subject_properties)
	{
		log() << " - " << kvp.first << " : " << kvp.second;
	}
	log() << "Public Key (" << cert.public_key_bit_size << " bits)";

	log() << "-------------------";
}

inline bool verify(bool preverified, asio::ssl::verify_context& ctx, ssl_certificate& cert)
{
	auto x509_cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	cert.version = utils::certversion(x509_cert);
	cert.serial_number = utils::serial(x509_cert);
	//    cert.issuer = utils::issuer_one_line(x509_cert);
	//    cert.issuer_properties = utils::issuer(x509_cert);
	//    cert.subject = utils::subject_one_line(x509_cert);
	//    cert.subject_properties = utils::subject(x509_cert);
	cert.public_key_bit_size = utils::public_key_size(x509_cert);

	auto error = X509_STORE_CTX_get_error(ctx.native_handle());
	switch(error)
	{
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
			log() << "Unable to get issuer certificate";
			break;
		case X509_V_ERR_CERT_NOT_YET_VALID:
		case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
			log() << "Certificate not yet valid!!";
			break;
		case X509_V_ERR_CERT_HAS_EXPIRED:
		case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
			log() << "Certificate expired..";
			break;
		case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			log() << "Self signed certificate!";
			preverified = true;
			break;
		default:
			break;
	}

	if(preverified)
	{
		cert.sha256 = std::vector<uint8_t>(EVP_MAX_MD_SIZE, 0);
		cert.sha512 = std::vector<uint8_t>(EVP_MAX_MD_SIZE, 0);

		const EVP_MD* digest256 = EVP_get_digestbyname("sha256");
		if(digest256)
		{
			unsigned int n{};
			X509_digest(x509_cert, digest256, cert.sha256.data(), &n);
			cert.sha256.resize(n);
		}

		const EVP_MD* digest512 = EVP_get_digestbyname("sha512");
		if(digest512)
		{
			unsigned int n{};
			X509_digest(x509_cert, digest512, cert.sha512.data(), &n);
			cert.sha512.resize(n);
		}
	}
	utils::log_cert(cert);

	return preverified;
}

inline asio::ssl::context::method to_asio_method(net::ssl_method method)
{
	switch(method)
	{
		case ssl_method::sslv2:
			return asio::ssl::context::method::sslv2;

		case ssl_method::sslv2_client:
			return asio::ssl::context::method::sslv2_client;

		case ssl_method::sslv2_server:
			return asio::ssl::context::method::sslv2_server;

		case ssl_method::sslv3:
			return asio::ssl::context::method::sslv3;

		case ssl_method::sslv3_client:
			return asio::ssl::context::method::sslv3_client;

		case ssl_method::sslv3_server:
			return asio::ssl::context::method::sslv3_server;

		case ssl_method::tlsv1:
			return asio::ssl::context::method::tlsv1;

		case ssl_method::tlsv1_client:
			return asio::ssl::context::method::tlsv1_client;

		case ssl_method::tlsv1_server:
			return asio::ssl::context::method::tlsv1_server;

		case ssl_method::sslv23:
			return asio::ssl::context::method::sslv23;

		case ssl_method::sslv23_client:
			return asio::ssl::context::method::sslv23_client;

		case ssl_method::sslv23_server:
			return asio::ssl::context::method::sslv23_server;

		case ssl_method::tlsv11:
			return asio::ssl::context::method::tlsv11;

		case ssl_method::tlsv11_client:
			return asio::ssl::context::method::tlsv11_client;

		case ssl_method::tlsv11_server:
			return asio::ssl::context::method::tlsv11_server;

		case ssl_method::tlsv12:
			return asio::ssl::context::method::tlsv12;

		case ssl_method::tlsv12_client:
			return asio::ssl::context::method::tlsv12_client;

		case ssl_method::tlsv12_server:
			return asio::ssl::context::method::tlsv12_server;

		case ssl_method::tls:
			return asio::ssl::context::method::tls;

		case ssl_method::tls_client:
			return asio::ssl::context::method::tls_client;

		case ssl_method::tls_server:
			return asio::ssl::context::method::tls_server;

		default:
			return asio::ssl::context::method::tlsv12;
	}
}
}
} // namespace net
