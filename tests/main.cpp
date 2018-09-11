#include "tcp_tests.h"
#include "tcp_ssl_tests.h"
#include "tcp_local_tests.h"

#include <iostream>

#include <netpp/service.h>
int main(int argc, char* argv[])
{
	net::init_services();

    if(argc != 3)
    {
        std::cerr << "Usage: server <tcp_ip> <listen_port>\n";
        return 1;
    }
    std::string ip = argv[1];
    std::string port = argv[2];
    
	//tcp::test(ip, port);
	tcp_ssl::test(ip, port);
	//tcp_local::test(ip, port);

	net::deinit_services();
	return 0;
}
