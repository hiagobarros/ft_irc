#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring> // Para strerror
#include <cerrno>  // Para errno
#include "Channel.hpp"

#define RED "\033[31m"
#define GRN "\033[32m"
#define YLW "\033[33m"
#define PUR "\033[34m"
#define BLE "\033[36m"
#define RST "\033[0m"

#include "Client.hpp"
class Server
{
    private:
        int                         _port;
        std::string                 _password;
        int                         _server_fd;
        std::vector<struct pollfd>  _fds;
        std::map<int, Client>       _clients;

        std::map<std::string, Channel> _channels; // Maps: channel name -> Channel object

        void setup();
        void acceptNewClient();
        void handleClientData(int client_fd);
        void removeClient(int client_fd);
    // Process client buffer, extract commands and execute them
        void processClientBuffer(int client_fd);
    // Execute a single IRC command
        void executeCommand(int client_fd, const std::string& command_line);

        bool isNicknameInUse(const std::string& nick);
        void checkRegistration(int client_fd);

        int getClientFdByNickname(const std::string& nick);

        void sendReply(int client_fd, int code, const std::string& target, const std::string& message);

    public:
        Server(int por, const std::string &password);
        ~Server();

        void run();
};

#endif