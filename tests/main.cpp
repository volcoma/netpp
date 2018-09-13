#include "tcp_local_tests.h"
#include "tcp_ssl_tests.h"
#include "tcp_tests.h"

#include <iostream>

#include <asiopp/service.h>

int main(int argc, char* argv[])
{
	net::init_services(std::thread::hardware_concurrency());

	if(argc < 2)
	{
		std::cerr << "Usage: <server/client/both>" << std::endl;
		return 1;
	}
	std::string what = argv[1];
    int count = 1;
    if(argc == 3)
    {
        count = atoi(argv[2]);
    }
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
		std::cerr << "Usage: <server/client/both>" << std::endl;
		return 1;
	}
	tcp::test(server, client, count);
	tcp_ssl::test(server, client, count);
	tcp_local::test(server, client, count);

	net::deinit_services();
	return 0;
}
