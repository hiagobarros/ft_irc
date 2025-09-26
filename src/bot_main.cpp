// NOVO ARQUIVO: bot_main.cpp
#include "Bot.hpp"

int main(int argc, char **argv) {
    if (argc != 4) {
        std::cerr << "Uso: ./bot <host> <porta> <senha_do_servidor>" << std::endl;
        return 1;
    }

    try {
        std::string host = argv[1];
        int port = std::atoi(argv[2]);
        std::string password = argv[3];

        Bot bot(host, port, password);
        bot.run();
    } catch (const std::exception& e) {
        std::cerr << "Erro fatal do Bot: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}