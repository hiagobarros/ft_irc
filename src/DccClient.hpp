#ifndef DCCCLIENT_HPP
#define DCCCLIENT_HPP

#include <iostream>
#include <string>
// Inclua todos os headers de rede necessários

class DccClient {
public:
    DccClient(const std::string& host, int port, const std::string& password, const std::string& nickname);
    ~DccClient();

    // O método principal que será chamado pelo main
    void run(const std::string& mode, const std::string& arg1, const std::string& arg2);

private:
    // Signal handling
    static bool _shutdown_requested;
    static void signalHandler(int signal);
    
    // Métodos para conexão com o servidor IRC
    void connectToIrc();
    void registerWithIrc();
    void sendMessageToIrc(const std::string& message);

    // Métodos para a lógica DCC
    void handleSend(const std::string& target_nick, const std::string& filepath);
    void handleReceive();
    void receiveFile(const std::string& filename, const std::string& ip, int port, long size);

    // Atributos
    std::string _host;
    int         _port;
    std::string _password;
    std::string _nickname;
    int         _irc_socket_fd;
};

#endif