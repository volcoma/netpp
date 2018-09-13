#include "tcp_ssl_tests.h"

#include <asiopp/service.h>
#include <netpp/messenger.h>

#include <atomic>
#include <functional>
#include <iostream>
#include <cassert>
namespace tcp
{
using namespace std::chrono_literals;

static std::atomic<net::connection::id_t> connection_id{0};
static std::atomic<net::connector::id_t> connector_id{0};

auto& get_network()
{
	static auto net = net::messenger::create();
	return net;
}

void test(bool server, bool client, int count)
{
	try
	{
		std::string host = "::1";
		uint16_t port = 2000;

		if(client)
		{
            for(int i = 0; i < count; ++i)
            {

                auto connector = net::create_tcp_client(host, std::to_string(port));

                if(!connector)
                {
                    return;
                }
                auto net = get_network();
                net->add_connector(connector,
                                   [](net::connection::id_t id) {
                                       std::cout << "client " << id << " connected" << std::endl;
                                       auto net = get_network();
                                       if(!net)
                                       {
                                           return;
                                       }
                                       net->send_msg(id, "ping");
                                   },
                                   [](net::connection::id_t id, const net::error_code& ec) {
                                       std::cout << "client " << id << " disconnected. Reason : " << ec.message()
                                                 << std::endl;
                                   },
                                   [](net::connection::id_t id, auto&& msg) {
                                       std::cout << "client " << id << " on_msg : " << msg << std::endl;
                                       std::this_thread::sleep_for(16ms);
                                       auto net = get_network();
                                       if(!net)
                                       {
                                           return;
                                       }
                                       if(msg == "ping")
                                       {
                                           net->send_msg(id, "pong");
                                       }
                                       else if(msg == "pong")
                                       {
                                           net->send_msg(id, "ping");
                                       }
                                   });
            }
		}

		if(server)
		{
			auto connector = net::create_tcp_server(port);

			if(!connector)
			{
				return;
			}
            auto net = get_network();
			connector_id = net->add_connector(connector,
											  [](net::connection::id_t id) {
												  std::cout << "server connected " << id << std::endl;
												  connection_id = id;
											  },
											  [](net::connection::id_t id, net::error_code ec) {
												  std::cout << "server client " << id
															<< " disconnected. reason : " << ec.message()
															<< std::endl;
											  },
											  [](net::connection::id_t id, auto&& msg) {
												  std::cout << "server on_msg : " << msg << std::endl;
												  std::this_thread::sleep_for(16ms);
                                                  auto net = get_network();
                                                  if(!net)
                                                  {
                                                      return;
                                                  }
												  if(msg == "ping")
												  {
													  net->send_msg(id, "pong");
												  }
												  else if(msg == "pong")
												  {
													  net->send_msg(id, "ping");
												  }
											  });
		}

        while(true)
		{
			std::this_thread::sleep_for(16ms);
            auto net = get_network();
			net->send_msg(1, "from_main");
			net->send_msg(connection_id, "from_main");
			static int i = 0;
			if(i++ % 100 == 0)
			{
				net->disconnect(connection_id);
			}
			if(i % 1000 == 0)
			{
				net->remove_connector(connector_id);
				break;
			}
		}

        get_network().reset();
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}
} // namespace tcp
