// NOVO ARQUIVO: Bot.hpp
#ifndef BOT_HPP
#define BOT_HPP

#include <iostream>
#include <string>
// ... inclua os headers de rede necessários ...

class Bot {
public:
    Bot(const std::string& host, int port, const std::string& password);
    ~Bot();

    void run();

private:
    // Signal handling
    static bool _shutdown_requested;
    static void signalHandler(int signal);
    void connectToServer();
    void registerWithServer();
    void mainLoop();
    void parseServerMessage(const std::string& message);
    void handleCommand(const std::string& channel, const std::string& command);
    void sendMessage(const std::string& message);

    std::string _host;
    int         _port;
    std::string _password;
    std::string _nickname;
    int         _socket_fd;

    std::string _quiz_answer;
};

#endif