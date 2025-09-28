#ifndef DCCCLIENT_HPP
#define DCCCLIENT_HPP

#include <iostream>
#include <string>
// Include all necessary network headers

class DccClient {
public:
    DccClient(const std::string& host, int port, const std::string& password, const std::string& nickname);
    ~DccClient();

    // The main method that will be called by main
    void run(const std::string& mode, const std::string& arg1, const std::string& arg2);

private:
    // Signal handling
    static bool _shutdown_requested;
    static void signalHandler(int signal);
    
    // Methods for IRC server connection
    void connectToIrc();
    void registerWithIrc();
    void sendMessageToIrc(const std::string& message);

    // Methods for DCC logic
    void handleSend(const std::string& target_nick, const std::string& filepath);
    void handleReceive();
    void receiveFile(const std::string& filename, const std::string& ip, int port, long size);

    // Attributes
    std::string _host;
    int         _port;
    std::string _password;
    std::string _nickname;
    int         _irc_socket_fd;
};

#endif