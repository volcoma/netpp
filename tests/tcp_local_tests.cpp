#include "tcp_local_tests.h"

#include <netpp/messenger.h>

#include <asiopp/service.h>

#include <atomic>
#include <functional>
#include <iostream>
namespace tcp_local
{
using namespace std::chrono_literals;

static std::atomic<net::connection::id_t> connection_id{0};
static std::atomic<net::connector::id_t> connector_id{0};

void test(bool server, bool client)
{
	auto net = net::messenger::create();

	try
	{
		std::remove("/tmp/test");

		if(client)
		{
			auto connector = net::create_tcp_local_client("/tmp/test");

            if(!connector)
            {
                return;
            }

			net->add_connector(connector,
							   [&net](net::connection::id_t id) {
								   std::cout << "client " << id << " connected" << std::endl;

								   net->send_msg(id, "ping");
							   },
							   [](net::connection::id_t id, const net::error_code& ec) {
								   std::cout << "client " << id << " disconnected. Reason : " << ec.message()
											 << std::endl;
							   },
							   [&net](net::connection::id_t id, auto&& msg) {
								   std::cout << "client " << id << " on_msg: " << msg << std::endl;
								   std::this_thread::sleep_for(16ms);

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

		if(server)
		{
			auto connector = net::create_tcp_local_server("/tmp/test");

            if(!connector)
            {
                return;
            }

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
											  [&net](net::connection::id_t id, auto&& msg) {
												  std::cout << "server on_msg: " << msg << std::endl;
												  std::this_thread::sleep_for(16ms);

												  if(msg == "ping")
												  {
													  net->send_msg(id, "pong");
												  }
												  else if(msg == "pong")
												  {
													  net->send_msg(id, "ping");
												  }

												  net->send_msg(id, "sadasdasdasdasdaghdh");
											  });
		}

		while(true)
		{
			std::this_thread::sleep_for(16ms);
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
				return;
			}
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
}
} // namespace tcp
