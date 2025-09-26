#include "DccClient.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>

// --- Construtor e Destrutor ---
DccClient::DccClient(const std::string& host, int port, const std::string& password, const std::string& nickname) :
    _host(host), _port(port), _password(password), _nickname(nickname), _irc_socket_fd(-1)
{}

DccClient::~DccClient() {
    if (_irc_socket_fd != -1) {
        close(_irc_socket_fd);
    }
}

// --- Lógica Principal ---
void DccClient::run(const std::string& mode, const std::string& arg1, const std::string& arg2) {
    // Primeiro, sempre nos conectamos ao servidor IRC
    connectToIrc();
    registerWithIrc();

    // Agora, dependendo do modo, executamos a ação
    if (mode == "send") {
        handleSend(arg1, arg2); // arg1 = target_nick, arg2 = filepath
    } else if (mode == "receive") {
        handleReceive();
    } else {
        std::cerr << "Invalid mode. Use 'send' or 'receive'." << std::endl;
    }

    std::cout << "DCC operation completed. Disconnecting from IRC." << std::endl;
}

// --- IRC Connection Methods (similar to Bot) ---
void DccClient::connectToIrc() {
    _irc_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_irc_socket_fd < 0) throw std::runtime_error("Error creating IRC socket");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    if (inet_pton(AF_INET, _host.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid host address");
    }

    if (connect(_irc_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Error connecting to IRC server");
    }
    std::cout << "Connected to IRC server as " << _nickname << "." << std::endl;
}

void DccClient::sendMessageToIrc(const std::string& message) {
    std::string full_message = message + "\r\n";
    send(_irc_socket_fd, full_message.c_str(), full_message.length(), 0);
}

void DccClient::registerWithIrc() {
    sendMessageToIrc("PASS " + _password);
    sendMessageToIrc("NICK " + _nickname);
    sendMessageToIrc("USER " + _nickname + " 0 * :" + _nickname);
    sleep(1); // Dá tempo para o servidor processar o registro
}

// --- Lógica de Envio DCC ---
void DccClient::handleSend(const std::string& target_nick, const std::string& filepath) {
    std::ifstream file(filepath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir o arquivo " << filepath << std::endl;
        return;
    }
    long file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) throw std::runtime_error("Error creating DCC listen socket");
    
    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = 0; // Porta aleatória

    bind(listen_fd, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
    listen(listen_fd, 1);

    socklen_t len = sizeof(listen_addr);
    getsockname(listen_fd, (struct sockaddr*)&listen_addr, &len);
    int port = ntohs(listen_addr.sin_port);

    std::cout << "Preparado para enviar. Aguardando conexão em 127.0.0.1 na porta " << port << "..." << std::endl;

    std::stringstream dcc_msg;
    dcc_msg << '\x01'<<"DCC SEND " << filepath.substr(filepath.find_last_of('/') + 1) << " 127.0.0.1 " << port << " " << file_size << "\x01";
    sendMessageToIrc("PRIVMSG " + target_nick + " :" + dcc_msg.str());
    
    int transfer_fd = accept(listen_fd, NULL, NULL);
    if (transfer_fd < 0) {
        std::cerr << "Error in DCC accept" << std::endl;
        close(listen_fd);
        return;
    }
    std::cout << "DCC connection accepted! Sending file..." << std::endl;

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        if (send(transfer_fd, buffer, file.gcount(), 0) < 0) {
            std::cerr << "Error sending file data." << std::endl;
            break;
        }
    }
    
    std::cout << "File transfer completed." << std::endl;
    file.close();
    close(transfer_fd);
    close(listen_fd);
}

// --- Lógica de Recebimento DCC ---
void DccClient::handleReceive() {
    std::cout << "Aguardando ofertas de DCC... (Pressione Ctrl+C para sair)" << std::endl;
    char buffer[4096];
    while (true) {
        int bytes_read = recv(_irc_socket_fd, buffer, 4096, 0);
        if (bytes_read <= 0) {
            std::cout << "Desconectado do servidor IRC." << std::endl;
            break;
        }
        buffer[bytes_read] = '\0';
        std::string message(buffer);

        if (message.find("PING") == 0) {
            sendMessageToIrc("PONG" + message.substr(4));
        }

        size_t dcc_pos = message.find("\001DCC SEND");
        if (dcc_pos != std::string::npos) {
            std::stringstream ss_dcc(message);
            std::string token, filename, ip_str, port_str, size_str;
            
            ss_dcc >> token >> token >> filename >> ip_str >> port_str >> size_str;
            size_str.erase(size_str.find_last_of('\x01')); // Remove o delimitador final
            
            std::cout << "Oferta DCC recebida para o arquivo: " << filename << std::endl;
            receiveFile(filename, ip_str, std::atoi(port_str.c_str()), std::atol(size_str.c_str()));
            break; // Sai depois de receber um arquivo
        }
    }
}

void DccClient::receiveFile(const std::string& filename, const std::string& ip, int port, long size) {
    int transfer_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (transfer_fd < 0) throw std::runtime_error("Error creating DCC transfer socket");

    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &send_addr.sin_addr);

    if (connect(transfer_fd, (struct sockaddr*)&send_addr, sizeof(send_addr)) < 0) {
        std::cerr << "Error connecting to receive file." << std::endl;
        return;
    }
    std::cout << "DCC connection established! Receiving file..." << std::endl;

    std::ofstream file(("received_" + filename).c_str(), std::ios::binary);
    
    char buffer[4096];
    long bytes_received = 0;
    while (bytes_received < size) {
        int bytes = recv(transfer_fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        file.write(buffer, bytes);
        bytes_received += bytes;
    }

    std::cout << "Arquivo '" << filename << "' recebido com sucesso (" << bytes_received << " bytes)." << std::endl;
    file.close();
    close(transfer_fd);
}