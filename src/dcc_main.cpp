#include "DccClient.hpp"

void printUsage() {
    std::cerr << "Uso:" << std::endl;
    std::cerr << "  Para enviar: ./dcc_client <host> <porta> <senha> <nick> send <nick_alvo> <caminho_do_arquivo>" << std::endl;
    std::cerr << "  Para receber: ./dcc_client <host> <porta> <senha> <nick> receive" << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 6) {
        printUsage();
        return 1;
    }

    try {
        std::string host = argv[1];
        int port = std::atoi(argv[2]);
        std::string password = argv[3];
        std::string nickname = argv[4];
        std::string mode = argv[5];

        DccClient client(host, port, password, nickname);

        if (mode == "send" && argc == 8) {
            std::string target_nick = argv[6];
            std::string filepath = argv[7];
            client.run(mode, target_nick, filepath);
        } else if (mode == "receive" && argc == 6) {
            client.run(mode, "", "");
        } else {
            printUsage();
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Erro fatal do DCC Client: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}