#include <atomic>
#include <chrono>
#include <iostream>

#include <asiopp/service.h>
#include <netpp/messenger.h>
#include <asiopp/utils.hpp>

using namespace std::chrono_literals;

static std::atomic<net::connection::id_t> connection_id{0};
static std::atomic<net::connector::id_t> connector_id{0};

void test(net::connector_ptr&& server, std::vector<net::connector_ptr>&& clients)
{

    connection_id = 0;
    connector_id = 0;

    for(auto& client : clients)
    {
        auto net = net::get_network();
        net->add_connector(client,
                           [](net::connection::id_t id) {
                               sout() << "client " << id << " connected" << "\n";
                               auto net = net::get_network();
                               if(!net)
                               {
                                   return;
                               }
                               net->send_msg(id, "echo");
                           },
                           [](net::connection::id_t id, const net::error_code& ec) {
                               sout() << "client " << id << " disconnected. Reason : " << ec.message()
                                         << "\n";
                           },
                           [](net::connection::id_t id, const auto& msg) {
                               sout() << "client " << id << " on_msg: " << msg << "\n";
                               std::this_thread::sleep_for(16ms);
                               auto net = net::get_network();
                               if(!net)
                               {
                                   return;
                               }

                               net->send_msg(id, net::byte_buffer(msg));

                           });
    }
    clients.clear();

    auto net = net::get_network();
    connector_id = net->add_connector(server,
                                      [](net::connection::id_t id) {
                                          sout() << "server connected " << id << "\n";
                                          connection_id = id;
                                      },
                                      [](net::connection::id_t id, net::error_code ec) {
                                          sout() << "server client " << id
                                                    << " disconnected. reason : " << ec.message()
                                                    << "\n";
                                      },
                                      [](net::connection::id_t id, auto&& msg) {
                                          sout() << "server on_msg: " << msg << "\n";
                                          std::this_thread::sleep_for(16ms);
                                          auto net = net::get_network();
                                          if(!net)
                                          {
                                              return;
                                          }

                                          net->send_msg(id, net::byte_buffer(msg));

                                      });
    server.reset();

    while(true)
    {
        std::this_thread::sleep_for(16ms);

        static int i = 0;
        if(i++ % 4 == 0)
        {
            // net->send_msg(1, "from_main");
            // net->send_msg(connection_id, "from_main");
        }

        if(i % 100 == 0)
        {
            net->disconnect(connection_id);
        }
        if(i % 1000 == 0)
        {
            net->remove_connector(connector_id);
            break;
        }
    }
    net->stop();
}

int main(int argc, char* argv[])
{

    if(argc < 2)
    {
        std::cerr << "Usage: <server/client/both>" << "\n";
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
        std::cerr << "Usage: <server/client/both>" << "\n";
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

    net::init_services(std::thread::hardware_concurrency());

    try
    {
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

//        {
//            std::vector<net::connector_ptr> clients;
//            if(is_client)
//            {
//                for(int i = 0; i < count; ++i)
//                {
//                    clients.emplace_back();
//                    auto& client = clients.back();
//                    client = net::create_tcp_local_client(domain);
//                }
//            }

//            net::connector_ptr server;
//            if(is_server)
//            {
//                server = net::create_tcp_local_server(domain);
//            }

//            test(std::move(server), std::move(clients));
//        }

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

//        {
//            std::vector<net::connector_ptr> clients;
//            if(is_client)
//            {
//                for(int i = 0; i < count; ++i)
//                {
//                    clients.emplace_back();
//                    auto& client = clients.back();
//                    client = net::create_tcp_ssl_local_client(domain, cert_file);
//                }
//            }

//            net::connector_ptr server;
//            if(is_server)
//            {
//                server = net::create_tcp_ssl_local_server(domain, cert_chain_file, private_key_file, dh_file);
//            }

//            test(std::move(server), std::move(clients));
//        }
    }
    catch(std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    net::deinit_messengers();
    net::deinit_services();
    return 0;
}
