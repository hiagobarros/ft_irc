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


/*
int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    typedef struct sockaddr_in t_address;
    typedef struct sockaddr t_sockaddr;

    
    std::vector<struct pollfd> fds;


    t_address address;
    try
    {
        if(server_fd < 0) throw std::runtime_error(RED"Socket create error");

            std::cout<<GRN<<"Socket successfully created"<<RST<<std::endl;

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(6667);

        if (bind(server_fd, (t_sockaddr *)&address, sizeof(address)) < 0) throw std::runtime_error(RED"bind error. port may be in use");

            std::cout<<"Bind successfully on port 6667"<<std::endl;

        if (listen(server_fd, 3) < 0) throw std::runtime_error(RED"Error listening...");
       // std::cout<<"Server lisening... wait for connections..."<<std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    struct pollfd server_pollfd;
    server_pollfd.fd = server_fd;
    server_pollfd.events = POLLIN;

    fds.push_back(server_pollfd);

    std::cout<<GRN<<"Server initialized, waiting for multiple clients..."<<RST<<std::endl;

    while (true)
    {
        int activity = poll(fds.data(), fds.size(), -1);
        if(activity < 0) {
            std::cerr<< "Poll error"<<std::endl;
            break;
        }
        if (fds[0].revents & POLLIN) {
            int new_socket_fd = accept(server_fd, NULL, NULL);
            if (new_socket_fd < 0) {
                std::cerr<<"Accept error"<<std::endl;
            } else {
                std::cout<<BLE"New connection fd = "<<new_socket_fd<<std::endl;
            
            
            struct pollfd new_client_pollfd;
            new_client_pollfd.fd = new_socket_fd;
            new_client_pollfd.events = POLLIN;

            fds.push_back(new_client_pollfd);
        }
    }

    for (size_t i = 1; i < fds.size(); ++i)
    {
        if(fds[i].revents & POLLIN)
        {
            char buffer[1024] = {0};
            int bytes_read = read(fds[i].fd, buffer, 1024);
            if(bytes_read <= 0)
            {
                std::cout<<RED<<"Client disconnected. fd ="<<fds[i].fd<<RST<<std::endl;
                close(fds[i].fd);
                fds.erase(fds.begin() + i);
                i--;
            } else {
                std::cout<<PUR"fd message "<<fds[i].fd<<": "<<buffer;
            }
        }
    }
}
    
    
   return (0); 
}
*/