#include <iostream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sys/socket.h"
#include "Client.hpp"
#include "Server.hpp"
#include <poll.h>
#include <vector>
#define RED "\033[31m"
#define GRN "\033[32m"
#define YLW "\033[33m"
#define PUR "\033[34m"
#define BLE "\033[36m"
#define RST "\033[0m"


int main(int argc, char **argv)
{
    if(argc != 3)
    {
        std::cerr<<"Use: ./ircserv <port> <password>"<<std::endl;
        return(1);
    }

    try
    {
        int port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535)
            throw std::invalid_argument("Invalid Port.");
        std::string passworld = argv[2];

        Server server(port, passworld);
        server.run();
    }
    catch(const std::exception& e)
    {
        std::cerr <<"Fatal error: "<< e.what() << '\n';
    }

    return (0);
}

