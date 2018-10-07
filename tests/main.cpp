#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <tuple>

#include <asiopp/service.h>
#include <builderpp/msg_builder.h>
#include <messengerpp/messenger.h>
#include <sstream>
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
		return byte_buffer{std::begin(str), std::end(str)};
	}
	static T from_buffer(byte_buffer&& buffer)
	{
		std::stringstream deserializer;
		deserializer << std::string{std::begin(buffer), std::end(buffer)};
		T msg{};
		deserializer >> msg;
		return msg;
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
//            net::log() << "server sending request to " << id;
//            auto f = net->send_request(id, "request");
//            auto msg = f.get();
//            net::log() << "server recieved response from " << id;
//            net->send_msg(id, std::move(msg));
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
		},
        [](net::connection::id_t id, itc::promise<std::string> promise, std::string msg)
        {
            net::log() << "client " << id << " on_request: " << msg;
            promise.set_value("echo");
        });
		// clang-format on
	}
	clients.clear();

	auto end = std::chrono::steady_clock::now() + 15s;
	while(!net->empty() && std::chrono::steady_clock::now() < end)
	{
		itc::this_thread::sleep_for(16ms);

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
		std::string address = "::1";
		std::string multicast_address = "ff31::8000:1234";
		std::string domain = "/tmp/test";
		std::string cert_file = CERT_DIR "ca.pem";
		std::string cert_chain_file = CERT_DIR "server.pem";
		std::string private_key_file = CERT_DIR "server.pem";
		std::string private_key_pass = "test";
		std::string dh_file = CERT_DIR "dh2048.pem";
		uint16_t port = 11111;
	};

	config conf;
	std::remove(conf.domain.c_str());
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
                return net::create_tcp_ssl_client(conf.address, conf.port, conf.cert_file);
            },
            [](const config& conf)
            {
                return net::create_tcp_ssl_server(conf.port, conf.cert_chain_file, conf.private_key_file, conf.dh_file, conf.private_key_pass);
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
                return net::create_tcp_ssl_local_client(conf.domain, conf.cert_file);
            },
            [](const config& conf)
            {
                return net::create_tcp_ssl_local_server(conf.domain, conf.cert_chain_file, conf.private_key_file, conf.dh_file, conf.private_key_pass);
            }
        )
    };
	// clang-format on

	itc::init();
	net::set_logger([](const std::string& msg) { std::cout << msg << std::endl; });
	net::init_services(std::thread::hardware_concurrency());

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
	itc::shutdown();
	return 0;
}
