#include <atomic>
#include <chrono>
#include <iostream>
#include <tuple>
#include <asiopp/service.h>
#include <messengerpp/messenger.h>
#include <netpp/logging.h>
// enum_map contains pairs of enum value and value string for each enum
// this shortcut allows us to use enum_map<whatever>.
template <typename ENUM>
using enum_map = std::map<ENUM, const std::string>;

// This variable template will create a map for each enum type which is
// instantiated with.
template <typename ENUM>
enum_map<ENUM> enum_values{};

template <typename ENUM>
void initialize() {}

template <typename ENUM, typename ... args>
void initialize(const ENUM value, const char *name, args ... tail)
{
    enum_values<ENUM>.emplace(value, name);
    initialize<ENUM>(tail ...);
}

using namespace std::chrono_literals;

std::vector<uint8_t> to_buffer(const std::string& str)
{
	return {std::begin(str), std::end(str)};
}

std::string from_buffer(const std::vector<uint8_t>& buffer)
{
	return {std::begin(buffer), std::end(buffer)};
}
static std::atomic<net::connection::id_t> server_con{0};
void test(net::connector_ptr&& server, std::vector<net::connector_ptr>&& clients)
{
	auto net = net::get_network();
	net->add_connector(server,
					   [](net::connection::id_t id) {
						   net::log() << "server connected " << id;
                           server_con = id;
						   auto net = net::get_network();
						   if(net)
						   {
							   net->send_msg(id, to_buffer("echo"));
						   }
					   },
					   [](net::connection::id_t id, net::error_code ec) {
						   net::log() << "server client " << id << " disconnected. reason : " << ec.message();
					   },
					   [](net::connection::id_t id, auto&& msg) {
						   net::log() << "server client " << id << " on_msg: " << from_buffer(msg);
						   std::this_thread::sleep_for(16ms);
						   auto net = net::get_network();
						   if(net)
						   {
							   net->send_msg(id, net::byte_buffer(msg));
						   }
					   });
	server.reset();
	for(auto& client : clients)
	{
		net->add_connector(client,
						   [](net::connection::id_t id) {
							   net::log() << "client " << id << " connected";

						   },
						   [](net::connection::id_t id, const net::error_code& ec) {
							   net::log()
								   << "client " << id << " disconnected. Reason : " << ec.message();
						   },
						   [](net::connection::id_t id, const auto& msg) {
							   net::log() << "client " << id << " on_msg: " << from_buffer(msg);
							   std::this_thread::sleep_for(16ms);
							   auto net = net::get_network();
							   if(net)
							   {
								   net->send_msg(id, net::byte_buffer(msg));
							   }

						   });
	}
	clients.clear();

	auto end = std::chrono::steady_clock::now() + 100s;
	while(!net->empty() && std::chrono::steady_clock::now() < end)
	{
		std::this_thread::sleep_for(16ms);

		static int i = 0;

//		if(i++ % 50 == 0)
//		{
//			net->send_msg(server_con, to_buffer("from_main"));
//		}
	}
    server_con = 0;
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
    struct config
    {
        std::string address = "::1";
        std::string multicast_address = "ff31::8000:1234";
        std::string domain = "/tmp/test";
        std::string cert_file = CERT_DIR "ca.pem";
        std::string cert_chain_file = CERT_DIR "server.pem";
        std::string private_key_file = CERT_DIR "server.pem";
        std::string dh_file = CERT_DIR "dh2048.pem";
        uint16_t port = 11111;
    };

	config conf;
	std::remove(conf.domain.c_str());

	net::set_logger([](const std::string& msg) { std::cout << msg << std::endl; });

	net::init_services(std::thread::hardware_concurrency());

    using creator = std::function<net::connector_ptr(config)>;
    std::vector<std::tuple<std::string, creator, creator>> creators =
    {
//        {
//            "UNICAST",
//            [](const config& conf)
//            {
//                return net::create_udp_unicast_client(conf.address, conf.port);
//            },
//            [](const config& conf)
//            {
//                return net::create_udp_unicast_server(conf.address, conf.port);
//            }
//        },
//        {
//            "MULTICAST",
//            [](const config& conf)
//            {
//                return net::create_udp_multicast_client(conf.multicast_address, conf.port);
//            },
//            [](const config& conf)
//            {
//                return net::create_udp_multicast_server(conf.multicast_address, conf.port);
//            }
//        },
//        {
//            "BROADCAST",
//            [](const config& conf)
//            {
//                return net::create_udp_broadcast_client(conf.port);
//            },
//            [](const config& conf)
//            {
//                return net::create_udp_broadcast_server(conf.port);
//            }
//        },
        {
            "TCP",
            [](const config& conf)
            {
                return net::create_tcp_client(conf.address, conf.port);
            },
            [](const config& conf)
            {
                return net::create_tcp_server(conf.port);
            }
        },
//        {
//            "TCP SSL",
//            [](const config& conf)
//            {
//                return net::create_tcp_ssl_client(conf.address, conf.port, conf.cert_file);
//            },
//            [](const config& conf)
//            {
//                return net::create_tcp_ssl_server(conf.port, conf.cert_chain_file, conf.private_key_file, conf.dh_file);
//            }
//        },
//        {
//            "TCP LOCAL",
//            [](const config& conf)
//            {
//                return net::create_tcp_local_client(conf.domain);
//            },
//            [](const config& conf)
//            {
//                return net::create_tcp_local_server(conf.domain);
//            }
//        },
//        {
//            "TCP SSL LOCAL",
//            [](const config& conf)
//            {
//                return net::create_tcp_ssl_local_client(conf.domain, conf.cert_file);
//            },
//            [](const config& conf)
//            {
//                return net::create_tcp_ssl_local_server(conf.domain, conf.cert_chain_file, conf.private_key_file, conf.dh_file);
//            }
//        }
    };


	try
	{
        for(const auto& creator : creators)
        {
            std::remove(conf.domain.c_str());
            const auto& name = std::get<0>(creator);
            const auto& client_creator = std::get<1>(creator);
            const auto& server_creator = std::get<2>(creator);

            net::log() << name;

			std::vector<net::connector_ptr> clients;
			if(is_client)
			{
				// only 1 of them will work with unicast
				for(int i = 0; i < count; ++i)
				{
					clients.emplace_back();
					auto& client = clients.back();
					client = client_creator(conf);
				}
			}
			net::connector_ptr server;
			if(is_server)
			{
				server = server_creator(conf);
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
