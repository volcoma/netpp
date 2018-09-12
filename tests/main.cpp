#include "tcp_tests.h"
#include "tcp_ssl_tests.h"
#include "tcp_local_tests.h"

#include <iostream>

#include <netpp/service.h>
int main(int argc, char* argv[])
{
	net::init_services();

    if(argc != 2)
    {
        std::cerr << "Usage: <server/client/both>" << std::endl;;
        return 1;
    }
    std::string what = argv[1];
    bool server = false;
    bool client = false;
    if(what == "server")
    {
        server = true;
    }
    else if(what == "client")
    {
        client = true;
    }
    else if(what == "both")
    {
        server = true;
        client = true;
    }
    else
    {
        std::cerr << "Usage: <server/client/both>" << std::endl;;
        return 1;
    }
	tcp::test(server, client);
	//tcp_ssl::test(server, client);
	//tcp_local::test(server, client);

	net::deinit_services();
	return 0;
}
