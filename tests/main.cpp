#include <atomic>
#include <chrono>
#include <iostream>

#include <asiopp/service.h>
#include <netpp/logging.h>
#include <netpp/messenger.h>

using namespace std::chrono_literals;

std::vector<uint8_t> to_buffer(const std::string& str)
{
	return {std::begin(str), std::end(str)};
}

std::string from_buffer(const std::vector<uint8_t>& buffer)
{
	return {std::begin(buffer), std::end(buffer)};
}

void test(net::connector_ptr&& server, std::vector<net::connector_ptr>&& clients)
{

	for(auto& client : clients)
	{
		auto net = net::get_network();
		net->add_connector(client,
						   [](net::connection::id_t id) {
							   net::log() << "client " << id << " connected\n";
							   auto net = net::get_network();
							   if(net)
							   {
								   net->send_msg(id, to_buffer("echoasss"));
							   }
						   },
						   [](net::connection::id_t id, const net::error_code& ec) {
							   net::log()
								   << "client " << id << " disconnected. Reason : " << ec.message() << "\n";
						   },
						   [](net::connection::id_t id, const auto& msg) {
							   net::log() << "client " << id << " on_msg: " << from_buffer(msg) << "\n";
							   std::this_thread::sleep_for(16ms);
							   auto net = net::get_network();
							   if(net)
							   {
								   net->send_msg(id, net::byte_buffer(msg));
							   }

						   });
	}
	clients.clear();

	auto net = net::get_network();
	net->add_connector(
		server, [](net::connection::id_t id) { net::log() << "server connected " << id << "\n"; },
		[](net::connection::id_t id, net::error_code ec) {
			net::log() << "server client " << id << " disconnected. reason : " << ec.message() << "\n";
		},
		[](net::connection::id_t id, auto&& msg) {
			net::log() << "server on_msg: " << from_buffer(msg) << "\n";
			std::this_thread::sleep_for(16ms);
			auto net = net::get_network();
			if(net)
			{
				net->send_msg(id, net::byte_buffer(msg));
			}
		});
	server.reset();

	auto end = std::chrono::steady_clock::now() + 20s;
	while(!net->empty() && std::chrono::steady_clock::now() < end)
	{
		std::this_thread::sleep_for(16ms);

		static int i = 0;

		if(i++ % 50 == 0)
		{
			// net->send_msg(1, "from_main");
		}
	}
	net->stop();
}

int main(int argc, char* argv[])
{

	if(argc < 2)
	{
		std::cerr << "Usage: <server/client/both>"
				  << "\n";
		return 1;
	}
	std::string what = argv[1];
	int count = 1;
	if(argc == 3)
	{
		count = atoi(argv[2]);
	}
	bool is_server = false;
	bool is_client = false;
	if(what == "server")
	{
		is_server = true;
	}
	else if(what == "client")
	{
		is_client = true;
	}
	else if(what == "both")
	{
		is_server = true;
		is_client = true;
	}
	else
	{
		std::cerr << "Usage: <server/client/both>"
				  << "\n";
		return 1;
	}
	std::string host = "::1";
	uint16_t port = 2000;
	std::string domain = "/tmp/test";
	std::string cert_file = CERT_DIR "ca.pem";
	std::string cert_chain_file = CERT_DIR "server.pem";
	std::string private_key_file = CERT_DIR "server.pem";
	std::string dh_file = CERT_DIR "dh2048.pem";
	std::remove(domain.c_str());

    net::set_logger([](const std::string& msg)
    {
        std::cout << msg;
    });

	net::init_services(std::thread::hardware_concurrency());

	try
	{

        // TCP SOCKET TEST
		{
			std::vector<net::connector_ptr> clients;
			if(is_client)
			{
				for(int i = 0; i < count; ++i)
				{
					clients.emplace_back();
					auto& client = clients.back();
					client = net::create_tcp_client(host, port);
				}
			}
			net::connector_ptr server;
			if(is_server)
			{
				server = net::create_tcp_server(port);
			}
			test(std::move(server), std::move(clients));
		}

        // DOMAIN SOCKTET TEST
		{
			std::vector<net::connector_ptr> clients;
			if(is_client)
			{
				for(int i = 0; i < count; ++i)
				{
					clients.emplace_back();
					auto& client = clients.back();
					client = net::create_tcp_local_client(domain);
				}
			}
			net::connector_ptr server;
			if(is_server)
			{
				server = net::create_tcp_local_server(domain);
			}
			test(std::move(server), std::move(clients));
		}

        // TCP SSL SOCKET TEST
		{
			std::vector<net::connector_ptr> clients;
			if(is_client)
			{
				for(int i = 0; i < count; ++i)
				{
					clients.emplace_back();
					auto& client = clients.back();
					client = net::create_tcp_ssl_client(host, port, cert_file);
				}
			}
			net::connector_ptr server;
			if(is_server)
			{
				server = net::create_tcp_ssl_server(port, cert_chain_file, private_key_file, dh_file);
			}
			test(std::move(server), std::move(clients));
		}

        // TCP SSL DOMAIN SOCKET TEST
		{
			std::vector<net::connector_ptr> clients;
			if(is_client)
			{
				for(int i = 0; i < count; ++i)
				{
					clients.emplace_back();
					auto& client = clients.back();
					client = net::create_tcp_ssl_local_client(domain, cert_file);
				}
			}
			net::connector_ptr server;
			if(is_server)
			{
				server = net::create_tcp_ssl_local_server(domain, cert_chain_file, private_key_file, dh_file);
			}
			test(std::move(server), std::move(clients));
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	net::deinit_messengers();
	net::deinit_services();
	return 0;
}
