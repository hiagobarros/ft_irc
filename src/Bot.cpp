// NOVO ARQUIVO: Bot.cpp
#include "Bot.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>

/////////????
#include <string>
#include <istream>
#include <sstream>
#include <cstring>
////////????

Bot::Bot(const std::string& host, int port, const std::string& password) :
    _host(host), _port(port), _password(password), _nickname("Bot"), _socket_fd(-1), _quiz_answer("")
{}

Bot::~Bot() {
    if (_socket_fd != -1) {
        close(_socket_fd);
    }
}

void Bot::connectToServer() {
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socket_fd < 0) {
        throw std::runtime_error("Error creating bot socket");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);

    // Convert host address (ex: "127.0.0.1") to network format
    if (inet_pton(AF_INET, _host.c_str(), &server_addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid host address");
    }

    // THE CLIENT MAGIC: connect()
    if (connect(_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Error connecting to server");
    }

    std::cout << "Bot successfully connected to server." << std::endl;
}




/////////////////////////////////////////////



// DENTRO DE Bot.cpp

void Bot::sendMessage(const std::string& message) {
    std::string full_message = message + "\r\n";
    send(_socket_fd, full_message.c_str(), full_message.length(), 0);
}

void Bot::registerWithServer() {
    sendMessage("PASS " + _password);
    sendMessage("NICK " + _nickname);
    sendMessage("USER " + _nickname + " 0 * :" + _nickname);
    // Wait a bit for registration to be processed (simple solution)
    sleep(1);
    sendMessage("JOIN #bot-channel"); // Channel where bot will operate
}

void Bot::mainLoop() {
    char buffer[4096];
    while (true) {
        int bytes_read = recv(_socket_fd, buffer, 4096, 0);
        if (bytes_read <= 0) {
            std::cout << "Server disconnected." << std::endl;
            break;
        }
        buffer[bytes_read] = '\0';
        parseServerMessage(buffer);
    }
}

void Bot::run() {
    connectToServer();
    registerWithServer();
    mainLoop();
}


// the bot understands what the server is saying and reacts

void Bot::parseServerMessage(const std::string& message) {
    std::stringstream ss(message);
    std::string line;

    while (std::getline(ss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }

        std::cout << "[Server]: " << line << std::endl;

        // Here send pong to server ping to not be disconnected
        if (line.find("PING") == 0) {
            sendMessage("PONG" + line.substr(4));
            continue;
        }

        size_t invite_pos = line.find("INVITE");
        if (invite_pos != std::string::npos) {
            // Parse: :<sender> INVITE <bot_nick> :<#channel>
            size_t channel_start = line.rfind(':'); // Channel name is the last parameter

            if (channel_start != std::string::npos) {
                std::string channel_name = line.substr(channel_start + 1);
                
                std::cout << "[Bot]: Received invitation to channel " << channel_name << ". Joining..." << std::endl;
                
                // Send JOIN command to enter the channel to which it was invited
                sendMessage("JOIN " + channel_name);
            }
            continue; // Skip to next line, as we already processed this one
        }

        // Look for chat messages (PRIVMSG) to respond to commands
        size_t privmsg_pos = line.find("PRIVMSG");
        if (privmsg_pos != std::string::npos) {
            // Parse: :<sender> PRIVMSG <#channel> :<text>
            size_t channel_start = line.find('#');
            size_t text_start = line.find(':', channel_start);

            if (channel_start != std::string::npos && text_start != std::string::npos) {
                std::string channel = line.substr(channel_start, text_start - channel_start - 1);
                std::string text = line.substr(text_start + 1);

                if (!text.empty() && text[0] == '!') {
                    handleCommand(channel, text.substr(1));
                }
            }
        }
    }
}


void Bot::handleCommand(const std::string& channel, const std::string& full_command) {
    // Separate command from argument
    std::string command;
    std::string args;
    size_t space_pos = full_command.find(' ');
    if (space_pos != std::string::npos) {
        command = full_command.substr(0, space_pos);
        args = full_command.substr(space_pos + 1);
    } else {
        command = full_command;
    }

    // *********** basic commands **********
    if (command == "help") {
        sendMessage("PRIVMSG " + channel + " :Hi! I am the Bot. Commands: !help, !joke, !hour, !dado [sides], !google [search], !quiz, !resposta [your_answer]");
    }
    else if (command == "joke") {
        sendMessage("PRIVMSG " + channel + " :What did the duck say to the duck? Quack Quack!");
    }
    else if (command == "hour") {
        time_t now = time(0);
        std::string dt = ctime(&now);
        dt.erase(dt.size() - 1); // Remove the \n
        sendMessage("PRIVMSG " + channel + " :Current time is: " + dt);
    }

    // !dado command
    else if (command == "dado") {
        srand(time(0)); // Initialize random number generator seed
        int sides = 6;
        if (!args.empty()) {
            sides = std::atoi(args.c_str());
        }
        if (sides <= 0) sides = 6;

        int result = (rand() % sides) + 1;
        
        std::stringstream ss;
        ss << "You rolled a " << sides << "-sided die and the result was: " << result;
        sendMessage("PRIVMSG " + channel + " :" + ss.str());
    }

    // !google command
    else if (command == "google") {
        if (args.empty()) {
            sendMessage("PRIVMSG " + channel + " :Please tell me what to search. Usage: !google <search term>");
        } else {
            std::string search_query = args;
            // Replace spaces with '+' for URL
            for (size_t i = 0; i < search_query.length(); ++i) {
                if (search_query[i] == ' ') {
                    search_query[i] = '+';
                }
            }
            sendMessage("PRIVMSG " + channel + " :Here is your search link: https://www.google.com/search?q=" + search_query);
        }
    }
    
    // Quiz commands (!quiz and !resposta)
    else if (command == "quiz") {
        if (!_quiz_answer.empty()) {
            sendMessage("PRIVMSG " + channel + " :A quiz is already in progress! What is the capital of Brazil?");
            return;
        }
        _quiz_answer = "brasilia"; // Set the answer (in lowercase)
        sendMessage("PRIVMSG " + channel + " :STARTING QUIZ! What is the capital of Brazil? Use !resposta <your_answer> to respond.");
    }
    else if (command == "resposta") {
        if (_quiz_answer.empty()) {
            sendMessage("PRIVMSG " + channel + " :No quiz is active at the moment. Use !quiz to start one!");
            return;
        }

        // Convert user answer to lowercase for comparison
        std::string user_answer = args;
        for (size_t i = 0; i < user_answer.length(); ++i) {
            user_answer[i] = std::tolower(user_answer[i]);
        }
        
        if (user_answer == _quiz_answer) {
            sendMessage("PRIVMSG " + channel + " :CONGRATULATIONS! You got the answer right!");
            _quiz_answer = ""; // Reset quiz
        } else {
            sendMessage("PRIVMSG " + channel + " :Wrong answer. Try again!");
        }
    }
}
