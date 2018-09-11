#include "tcp_ssl_tests.h"

#include <netpp/messenger.h>
#include <netpp/service.h>

#include <netpp/tcp_client.h>
#include <netpp/tcp_server.h>

#include <functional>
#include <iostream>
namespace tcp
{
using namespace std::chrono_literals;
net::connector_ptr create_tcp_server(uint16_t port)
{
    auto net_context = net::context();
    net::tcp::endpoint listen_endpoint(net::tcp::v6(), port);
    return std::make_shared<net::tcp_server>(*net_context, listen_endpoint);
}

net::connector_ptr create_tcp_client(const std::string& host, const std::string& port)
{
    auto net_context = net::context();
    net::tcp::resolver r(*net_context);
    auto res = r.resolve(host, port);
    auto endpoint = res.begin()->endpoint();
    return std::make_shared<net::tcp_client>(*net_context, endpoint);
}

static std::atomic<net::connection::id> connection_id{0};
static std::atomic<net::connector::id> connector_id{0};
void test(bool server, bool client)
{
    net::messenger net;

    try
    {
        std::string host = "::1";
        uint16_t port = 2000;

        if(client)
        {
            auto connector = create_tcp_client(host, std::to_string(port));
            net.add_connector(connector,
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
        }

        if(server)
        {
            auto connector = create_tcp_server(port);
            connector_id = net.add_connector(connector,
                                             [](net::connection::id id) {
                                                 std::cout << "server connected " << id << std::endl;
                                                 connection_id = id;
                                             },
                                             [](net::connection::id id, asio::error_code ec) {
                                                 std::cout << "server client " << id
                                                           << " disconnected. reason : " << ec.message()
                                                           << std::endl;
                                             },
                                             [&net](net::connection::id id, auto&& msg) {
                                                 std::cout << "server on_msg: " << msg << std::endl;
                                                 std::this_thread::sleep_for(16ms);

                                                 if(msg == "ping")
                                                 {
                                                     net.send_msg(id, "pong");
                                                 }
                                                 else if(msg == "pong")
                                                 {
                                                     net.send_msg(id, "ping");
                                                 }

                                                 net.send_msg(id, "sadasdasdasdasdaghdh");
                                             });
        }

        while(true)
        {
            std::this_thread::sleep_for(16ms);
            net.send_msg(1, "from_main");

            net.send_msg(connection_id, "from_main");
            static int i = 0;
            if(i++ % 10 == 0)
            {
                net.disconnect(connection_id);
            }
            if(i % 1000 == 0)
            {
                net.remove_connector(connector_id);
                break;
            }
        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
} // namespace tcp
