#include "tcp_ssl_tests.h"

#include <netpp/service.h>
#include <netpp/messenger.h>

#include <netpp/tcp_ssl_client.h>
#include <netpp/tcp_ssl_server.h>

#include <functional>
#include <iostream>
namespace tcp_ssl
{
using namespace std::chrono_literals;

net::connector_ptr create_tcp_ssl_server(uint16_t port, const std::string& cert_chain_file,
										 const std::string& private_key_file, const std::string dh_file)
{
	auto net_context = net::context();
	net::tcp::endpoint listen_endpoint(net::tcp::v6(), port);
	return std::make_shared<net::tcp_ssl_server>(*net_context, listen_endpoint, cert_chain_file,
												 private_key_file, dh_file);
}

net::connector_ptr create_tcp_ssl_client(const std::string& host, const std::string& port, const std::string& cert_file)
{
	auto net_context = net::context();
	net::tcp::resolver r(*net_context);
	auto res = r.resolve(host, port);
    auto endpoint = res.begin()->endpoint();
	return std::make_shared<net::tcp_ssl_client>(*net_context, endpoint, cert_file);
}


void test(const std::string& ip, const std::string& port)
{
	net::messenger net;

	try
	{

		auto client = create_tcp_ssl_client(ip, port, "ca.pem");
		net.add_connector(client,
						  [&net](net::connection::id id) {
							  std::cout << "client " << id << " connected" << std::endl;

							  net.send_msg(id, "ping");
						  },
						  [](net::connection::id id, asio::error_code ec) {
							  std::cout << "client " << id << " disconnected. Reason : " << ec.message()
										<< std::endl;
						  },
						  [&net](net::connection::id id, auto&& msg) {
							  std::cout << "client " << id << " on_msg: " << msg << std::endl;
							  std::this_thread::sleep_for(16ms);

							  if(msg == "ping")
							  {
								  net.send_msg(id, "pong");
							  }
							  else if(msg == "pong")
							  {
								  net.send_msg(id, "ping");
							  }
						  });

//		auto server = create_tcp_ssl_server(atoi(port.c_str()), "server.pem", "server.pem", "dh2048.pem");
//		net.add_connector(server,
//						  [](net::connection::id id) { std::cout << "server connected " << id << std::endl; },
//						  [](net::connection::id id, asio::error_code ec) {
//							  std::cout << "server client " << id
//										<< " disconnected. reason : " << ec.message() << std::endl;
//						  },
//						  [server = server.get()](net::connection::id id, auto&& msg) {
//							  std::cout << "server on_msg: " << msg << std::endl;
//							  std::this_thread::sleep_for(16ms);

//							  if(msg == "ping")
//							  {
//								  server->send_msg(id, "pong");
//							  }
//							  else if(msg == "pong")
//							  {
//								  server->send_msg(id, "ping");
//							  }

//							  server->send_msg(id, "sadasdasdasdasdaghdh");
//						  });

		while(true)
		{
			std::this_thread::sleep_for(16ms);
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

}
} // namespace tcp
