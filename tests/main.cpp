#include <asiopp/service.h>
#include <messengerpp/messenger.h>
#include <builderpp/msg_builder.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>

using namespace std::chrono_literals;

namespace net
{

template <typename T>
struct serializer<T, std::stringstream, std::stringstream>
{
	static byte_buffer to_buffer(const T& msg)
	{
		std::stringstream serializer;
		serializer << msg;
		auto str = serializer.str();
		return {std::begin(str), std::end(str)};
	}
	static T from_buffer(byte_buffer&& buffer)
	{
		std::stringstream deserializer({std::begin(buffer), std::end(buffer)});
		T msg{};
		deserializer >> msg;
		return msg;
	}
};

template <>
struct serializer<std::string, std::stringstream, std::stringstream>
{
	static byte_buffer to_buffer(const std::string& msg)
	{
		return {std::begin(msg), std::end(msg)};
	}
	static std::string from_buffer(byte_buffer&& buffer)
	{
		return {std::begin(buffer), std::end(buffer)};
	}
};

template <typename T>
auto get_network()
{
	return get_messenger<T, std::stringstream, std::stringstream>();
}
}

static std::atomic<net::connection::id_t> server_con{0};

void setup_connector(net::connector_ptr& connector)
{
	if(connector)
	{
		connector->create_builder = net::msg_builder::get_creator<net::single_buffer_builder>();
	}
}

void run_test(net::connector_ptr&& server, std::vector<net::connector_ptr>&& clients)
{
	auto net = net::get_network<std::string>();
	// clang-format off
    setup_connector(server);
	net->add_connector(server,
    [](net::connection::id_t id)
    {
        net::log() << "server connected " << id;
        server_con = id;
        auto net = net::get_network<std::string>();
        if(net)
        {
            net->send_msg(id, "echo");
        }
    },
    [](net::connection::id_t id, net::error_code ec)
    {
        net::log() << "server client " << id << " disconnected. reason : " << ec.message();
        if(id == server_con)
        {
            server_con = 0;
        }
    },
    [](net::connection::id_t id, std::string msg)
    {
        net::log() << "server client " << id << " on_msg: " << msg;
        //itc::this_thread::sleep_for(16ms);
        auto net = net::get_network<std::string>();
        if(net)
        {
            net->send_msg(id, std::move(msg));
        }
    });
	// clang-format on

	server.reset();

	for(auto& client : clients)
	{
		setup_connector(client);
		// clang-format off
		net->add_connector(client,
		[](net::connection::id_t id)
        {
		    net::log() << "client " << id << " connected";
		},
		[](net::connection::id_t id, const net::error_code& ec)
        {
		    net::log() << "client " << id << " disconnected. Reason : " << ec.message();
		},
		[](net::connection::id_t id, std::string msg)
        {
		    net::log() << "client " << id << " on_msg: " << msg;
		    //itc::this_thread::sleep_for(16ms);
            auto net = net::get_network<std::string>();
		    if(net)
		    {
		 	   net->send_msg(id, std::move(msg));
		    }
		});
		// clang-format on
	}
	clients.clear();

	auto end = std::chrono::steady_clock::now() + 15s;
	while(std::chrono::steady_clock::now() < end)
	{
		static int i = 0;

		if(i++ % 50 == 0)
		{
			net->send_msg(server_con, "from_main");
		}

		if(i % 100 == 0)
		{
			net->disconnect(server_con);
		}
	}
	server_con = 0;
	net->remove_all();
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
		std::string address{};
		std::string multicast_address{};
		std::string domain{};
		std::string cert_auth_file{};
		std::string cert_chain_file{};
		std::string private_key_file{};
		std::string private_key_pass{};
		std::string dh_file{};
		uint16_t port{};

		net::ssl_config ssl_client{};
		net::ssl_config ssl_server{};
	};

	config conf;
	conf.address = "::1";
	conf.multicast_address = "ff31::8000:1234";
	conf.domain = "/tmp/test";
	conf.port = 11111;

	conf.ssl_client.cert_auth_file = CERT_DIR "ca.pem";

	conf.ssl_client.cert_chain_file = CERT_DIR "server.pem";
	conf.ssl_client.private_key_file = CERT_DIR "server.pem";
	conf.ssl_client.dh_file = CERT_DIR "dh2048.pem";
	conf.ssl_client.private_key_password = "test";

	conf.ssl_server.cert_auth_file = CERT_DIR "ca.pem";

	conf.ssl_server.cert_chain_file = CERT_DIR "server.pem";
	conf.ssl_server.private_key_file = CERT_DIR "server.pem";
	conf.ssl_server.dh_file = CERT_DIR "dh2048.pem";
	conf.ssl_server.private_key_password = "test";

	// clang-format off

    using creator = std::function<net::connector_ptr(config)>;
    std::vector<std::tuple<std::string, creator, creator>> creators =
    {
        std::make_tuple
        (
            "UNICAST",
            [](const config& conf)
            {
                return net::create_udp_unicast_client(conf.address, conf.port);
            },
            [](const config& conf)
            {
                return net::create_udp_unicast_server(conf.address, conf.port);
            }
        ),
        std::make_tuple
        (
            "MULTICAST",
            [](const config& conf)
            {
                return net::create_udp_multicast_client(conf.multicast_address, conf.port);
            },
            [](const config& conf)
            {
                return net::create_udp_multicast_server(conf.multicast_address, conf.port);
            }
        ),
        std::make_tuple
        (
            "BROADCAST",
            [](const config& conf)
            {
                return net::create_udp_broadcast_client(conf.port);
            },
            [](const config& conf)
            {
                return net::create_udp_broadcast_server(conf.port);
            }
        ),
        std::make_tuple
        (
            "TCP",
            [](const config& conf)
            {
                return net::create_tcp_client(conf.address, conf.port);
            },
            [](const config& conf)
            {
                return net::create_tcp_server(conf.port);
            }
        ),
        std::make_tuple
        (
            "TCP SSL",
            [](const config& conf)
            {
                return net::create_tcp_ssl_client(conf.address, conf.port, conf.ssl_client);
            },
            [](const config& conf)
            {
                net::ssl_config ssl_config;
                return net::create_tcp_ssl_server(conf.port, conf.ssl_server);
            }
        ),
        std::make_tuple
        (
            "TCP LOCAL",
            [](const config& conf)
            {
                return net::create_tcp_local_client(conf.domain);
            },
            [](const config& conf)
            {
                return net::create_tcp_local_server(conf.domain);
            }
        ),
        std::make_tuple
        (
            "TCP SSL LOCAL",
            [](const config& conf)
            {
                return net::create_tcp_ssl_local_client(conf.domain, conf.ssl_client);
            },
            [](const config& conf)
            {
                return net::create_tcp_ssl_local_server(conf.domain, conf.ssl_server);
            }
        )
    };
	// clang-format on

	auto info_logger = [](const std::string& msg) { std::cout << msg << std::endl; };

	net::set_logger(info_logger);
	net::init_services();

	try
	{
		for(int i = 0; i < 20; ++i)
		{

			for(const auto& creator : creators)
			{
				std::remove(conf.domain.c_str());
				const auto& name = std::get<0>(creator);
				const auto& client_creator = std::get<1>(creator);
				const auto& server_creator = std::get<2>(creator);
				net::log() << "-------------";
				net::log() << name;
				net::log() << "-------------";

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
				run_test(std::move(server), std::move(clients));
			}
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
