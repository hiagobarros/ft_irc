#include "Server.hpp"
//#include "Client.hpp"

#include <sstream>

Server::Server(int port, const std::string &password) : _port(port), _password(password), _server_fd(-1)
{
    setup();
}

Server::~Server()
{
    for (size_t i = 0; i < _fds.size(); ++i)
    {
        close(_fds[i].fd);
    }
}

void Server::setup()
{

    _server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_server_fd < 0) {
        // If socket creation fails, throw exception IMMEDIATELY.
        throw std::runtime_error("Error creating socket: " + std::string(strerror(errno)));
    }

    std::cout << "DEBUG: socket() created successfully. fd = " << _server_fd << std::endl;

    // Optional but recommended: allows reusing address and port immediately
    int opt = 1;
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        // This error will only occur if _server_fd is valid but there's another problem.
        throw std::runtime_error("Error in setsockopt: " + std::string(strerror(errno)));
    }

    struct sockaddr_in address;
    typedef struct sockaddr t_sockaddr;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if(bind(_server_fd, (t_sockaddr *)&address, sizeof(address) ) < 0)
        throw std::runtime_error("Bind error: " + std::string(strerror(errno)));
    
    if (listen(_server_fd, 10) < 0)
        throw std::runtime_error("Listen error: " + std::string(strerror(errno)));
    
    struct pollfd server_pollfd;
    server_pollfd.fd = _server_fd;
    server_pollfd.events = POLLIN;
    server_pollfd.revents = 0;
    _fds.push_back(server_pollfd);

    std::cout<<"Server listening on PORT: "<<_port<<std::endl;
}

void Server::run() {
    while (true) {
        int activity = poll(_fds.data(), _fds.size(), -1);
        if (activity < 0) {
            std::cerr << "Poll error: " << strerror(errno) << std::endl;
            break;
        }

        // Still check for new connections first
        if (_fds[0].revents & POLLIN) {
            acceptNewClient();
        }

        // --- CRUCIAL CHANGE HERE ---
        // Iterate from back to front to safely remove clients.
        // The loop starts at the last element and goes to the second (index 1).
        // Element at index 0 is the server, which is never removed here.
        for (size_t i = _fds.size() - 1; i > 0; --i) {
            if (_fds[i].revents & POLLIN) {
                // CRITICAL FIX: Check if client still exists before processing
                if (_clients.find(_fds[i].fd) != _clients.end()) {
                    handleClientData(_fds[i].fd);
                }
            }
        }
    }
}


void Server::acceptNewClient()
{
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
    int new_client_fd = accept(_server_fd, (struct sockaddr *)&client_address, &client_len);

    if(new_client_fd < 0) 
    {
        std::cerr << "Accept error: "<<strerror(errno)<<std::endl;
        return;
    }

    struct pollfd new_pollfd;
    new_pollfd.fd = new_client_fd;
    new_pollfd.events = POLLIN;
    new_pollfd.revents = 0;
    _fds.push_back(new_pollfd);

    _clients.insert(std::make_pair(new_client_fd, Client(new_client_fd)));

    std::cout<<"New IP connection: "<<inet_ntoa(client_address.sin_addr)
                <<" at PORT: "<<ntohs(client_address.sin_port)
                <<" | FD: "<<new_client_fd<<std::endl;
    send(new_client_fd, "Type: PASS <server's password>\n", sizeof("Type: PASS <server's password>\n"), 0);
}


void Server::handleClientData(int client_fd) {
    // CRITICAL FIX: Check if client still exists before processing
    if (_clients.find(client_fd) == _clients.end()) {
        return; // Client was already removed, skip processing
    }
    
    char temp_buffer[1024] = {0};
    int bytes_read = recv(client_fd, temp_buffer, 1024, 0);

    if (bytes_read <= 0) {
        if (bytes_read == 0)
            std::cout << "Client (FD: " << client_fd << ") disconnected." << std::endl;
        else
            std::cerr << "Recv error for FD " << client_fd << ": " << strerror(errno) << std::endl;
        removeClient(client_fd);
    } else {
        // CRITICAL FIX: Check again before accessing client data
        if (_clients.find(client_fd) != _clients.end()) {
            // Get client buffer and append new data
            _clients[client_fd].getBuffer().append(temp_buffer, bytes_read);
            // Now process buffer to see if we have complete commands
            processClientBuffer(client_fd);
        }
    }
}

void Server::processClientBuffer(int client_fd) {
    // CRITICAL FIX: Check if client still exists before processing
    if (_clients.find(client_fd) == _clients.end()) {
        return; // Client was already removed, skip processing
    }
    
    // CRITICAL FIX: Make a copy of the buffer to avoid use-after-free
    std::string buffer_copy = _clients[client_fd].getBuffer();
    
    // Clear the original buffer to avoid processing the same data twice
    _clients[client_fd].getBuffer().clear();
    
    // An IRC command ends with "\r\n". We look for this.
    size_t pos = 0;
    while ((pos = buffer_copy.find("\r\n")) != std::string::npos || (pos = buffer_copy.find("\n")) != std::string::npos) {//???<\r\n>
        // CRITICAL FIX: Check if client still exists before processing each command
        if (_clients.find(client_fd) == _clients.end()) {
            return; // Client was removed, stop processing
        }
        
        // Extract command line
        std::string command_line = buffer_copy.substr(0, pos);
        // Remove command line (and \r\n) from buffer_copy
        if((pos = buffer_copy.find("\r\n")) != std::string::npos)
            buffer_copy.erase(0, pos + 2);//???
        else if((pos = buffer_copy.find("\n")) != std::string::npos)
            buffer_copy.erase(0, pos + 1);//???/
        // If line is not empty, execute it
        if (!command_line.empty()) {
            executeCommand(client_fd, command_line);
        }
    }
    
    // CRITICAL FIX: If client still exists, put back any remaining data
    if (_clients.find(client_fd) != _clients.end()) {
        _clients[client_fd].getBuffer() = buffer_copy;
    }
}


void Server::executeCommand(int client_fd, const std::string& command_line) {
    std::cout << "DEBUG: Received from FD " << client_fd << ": " << command_line << std::endl;

    std::string command;
    std::string params;
    size_t space_pos = command_line.find(' ');
    if (space_pos != std::string::npos) {
        command = command_line.substr(0, space_pos);
        params = command_line.substr(space_pos + 1);
    } else {
        command = command_line;
    }

    Client& client = _clients[client_fd];
    std::string client_nick = client.getNickname().empty() ? "*" : client.getNickname();
    // Registration commands (always allowed before registering)
    if (command == "PASS") {
        if (client.isPasswordCorrect()) {
            std::cout << "WARNING: Client " << client_fd << " tried to send PASS twice.\r\n" << std::endl;
            return;
        }
        if (params == _password) {
            std::cout << "INFO: Client " << client_fd << " provided correct password." << std::endl;
            std::string correct_msg = "correct password!!!\r\n";
            send(client_fd, correct_msg.c_str() , correct_msg.length(), 0);
            client.setPasswordCorrect(true);
        } else {
            std::cout << "ERROR: Client " << client_fd << " provided wrong password. Disconnecting." << std::endl;
            std::string error_msg = "ERROR :Closing link: Invalid password\r\n";
            send(client_fd, error_msg.c_str(), error_msg.length(), 0);
            removeClient(client_fd);
        }
    }
    else if (command == "NICK") {
        if (!client.isPasswordCorrect()) {
            removeClient(client_fd);
            return;
        }
        if (params.empty()) {
            sendReply(client_fd, 431, client_nick, ":No nickname given"); // ERR_NONICKNAMEGIVEN
            //std::cout << "ERRO: Cliente " << client_fd << " tentou NICK sem parâmetro." << std::endl;
            return;
        }
        if (isNicknameInUse(params)) {
            sendReply(client_fd, 433, client_nick, params + " :Nickname is already in use"); // ERR_NICKNAMEINUSE
            //std::cout << "ERRO: Nickname '" << params << "' já está em uso." << std::endl;
            return;
        }
        client.setNickname(params);
        std::cout << "INFO: Client " << client_fd << " set nickname to " << params << std::endl;
        checkRegistration(client_fd);
    }
    else if (command == "USER") {
        if (!client.isPasswordCorrect()) {
            removeClient(client_fd);
            return;
        }
        size_t first_space = params.find(' ');
        if (first_space == std::string::npos) {
            sendReply(client_fd, 461, client_nick, "USER :Not enough parameters"); // ERR_NEEDMOREPARAMS
            //std::cout << "ERRO: Cliente " << client_fd << " enviou USER com parâmetros insuficientes." << std::endl;
            return;
        }
        std::string username = params.substr(0, first_space);
        client.setUsername(username);
        std::cout << "INFO: Client " << client_fd << " set username to " << username << std::endl;
        checkRegistration(client_fd);
    }
    else if (command == "QUIT") {
        std::string quit_message = params.empty() ? "Client Quit" : params.substr(params.find(':') + 1);
        
        std::cout << "INFO: " << client.getNickname() << " (FD: " << client_fd << ") is leaving. (" << quit_message << ")" << std::endl;

        // Monta a notificação de QUIT
        std::string quit_notification = ":" + client.getNickname() + " QUIT :" + quit_message + "\r\n";
        
        // Envia a notificação para todos os canais em que o cliente está
        // É importante fazer uma cópia dos canais ou iterar com cuidado
        std::vector<std::string> channels_to_notify;
        for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
            if (it->second.isClientInChannel(client_fd)) {
                it->second.broadcastMessage(quit_notification, client_fd);
            }
        }
        
        // Remove o cliente do servidor
        removeClient(client_fd);
    }
    // Check if client is ALREADY REGISTERED to process other commands
    else if (client.isRegistered()) {
        if (command == "JOIN") {
            if (params.empty()) {
                // Send error: ERR_NEEDMOREPARAMS (461)
                // Ex: :irc.42.fr 461 user JOIN :Not enough parameters
                return;
            }

            // Parse: JOIN <#channel> [key]
            std::string channel_name = params.substr(0, params.find(' '));
            std::string key = (params.find(' ') != std::string::npos) ? params.substr(params.find(' ') + 1) : "";

            if (channel_name.empty() || channel_name[0] != '#') {
                // Comportamento inválido, pode ignorar ou enviar um erro genérico
                return;
            }

            // Find or create channel
            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) {
                _channels.insert(std::make_pair(channel_name, Channel(channel_name)));
                std::cout << "INFO: Channel " << channel_name << " created." << std::endl;
                it = _channels.find(channel_name);
            }

            // Get a reference to the Channel object
            Channel& channel = it->second;

            // --- MODE CHECKS START HERE ---

            // 1. Is channel full? (Mode +l)
            if (channel.getUserLimit() > 0 && channel.getClients().size() >= (size_t)channel.getUserLimit()) {
                // Send error: ERR_CHANNELISFULL (471)
                sendReply(client_fd, 471, client_nick, channel_name + " :Cannot join channel (+l)"); // ERR_CHANNELISFULL
                //std::string error_msg = ":irc.42.fr 471 " + client.getNickname() + " " + channel_name + " :Cannot join channel (+l)\r\n";
                //send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                return;
            }

            // 2. Does channel require a password (key)? (Mode +k)
            if (channel.hasKey() && channel.getKey() != key) {
                // Send error: ERR_BADCHANNELKEY (475)
                sendReply(client_fd, 475, client_nick, channel_name + " :Cannot join channel (+k)"); // ERR_BADCHANNELKEY
                //std::string error_msg = ":irc.42.fr 475 " + client.getNickname() + " " + channel_name + " :Cannot join channel (+k)\r\n";
                //send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                return;
            }

            // 3. Is channel invite-only? (Mode +i)
            if (channel.isInviteOnly() && !channel.isInvited(client_fd)) {
                // Send error: ERR_INVITEONLYCHAN (473)
                sendReply(client_fd, 473, client_nick, channel_name + " :Cannot join channel (+i)"); // ERR_INVITEONLYCHAN
                //std::string error_msg = ":irc.42.fr 473 " + client.getNickname() + " " + channel_name + " :Cannot join channel (+i)\r\n";
                //send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                return;
            }
            
            // --- END OF MODE CHECKS ---

            // If passed all checks, add client
            channel.addClient(client_fd);
            std::cout << "INFO: Client " << client.getNickname() << " joined channel " << channel_name << std::endl;

            // Notify client that they joined and who else is there
            // 1. Send JOIN confirmation to the client AND to all others in the channel
            std::string join_notification = ":" + client.getNickname() + " JOIN " + channel_name + "\r\n";
            const std::vector<int>& clients_in_channel_after_join = channel.getClients();
            for (size_t i = 0; i < clients_in_channel_after_join.size(); ++i) {
                send(clients_in_channel_after_join[i], join_notification.c_str(), join_notification.length(), 0);
            }

            // 2. Send current TOPIC to the client who just joined (RPL_TOPIC)
            std::string topic_msg = ":irc.42.fr 332 " + client.getNickname() + " " + channel_name + " :" + channel.getTopic() + "\r\n";
            send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);

            // 3. Send list of users in channel to the client who just joined (RPL_NAMREPLY)
            std::string names_list = ":irc.42.fr 353 " + client.getNickname() + " = " + channel_name + " :";
            const std::vector<int>& all_clients_in_channel = channel.getClients();
            for (size_t i = 0; i < all_clients_in_channel.size(); ++i) {
                if (channel.isOperator(all_clients_in_channel[i])) {
                    names_list += "@"; // Add '@' for operators
                }
                names_list += _clients[all_clients_in_channel[i]].getNickname() + " ";
            }
            names_list += "\r\n";
            send(client_fd, names_list.c_str(), names_list.length(), 0);

            // 4. End of names list (RPL_ENDOFNAMES)
            std::string end_names_msg = ":irc.42.fr 366 " + client.getNickname() + " " + channel_name + " :End of /NAMES list.\r\n";
            send(client_fd, end_names_msg.c_str(), end_names_msg.length(), 0);
        } 
        else if (command == "PRIVMSG") {
            size_t colon_pos = params.find(':');
            if (colon_pos == std::string::npos) {
                // Send error: ERR_NOTEXTTOSEND (412)
                std::string error_msg = ":irc.42.fr 412 " + client.getNickname() + " :No text to send\r\n";
                send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                return;
            }
            std::string target = params.substr(0, colon_pos > 0 ? colon_pos - 1 : 0);
            std::string message_text = params.substr(colon_pos + 1);

            if (target.empty()) {
                // Send error: ERR_NORECIPIENT (411)
                std::string error_msg = ":irc.42.fr 411 " + client.getNickname() + " :No recipient given (" + command + ")\r\n";
                send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                return;
            }

            if (target[0] == '#') { // Message to a Channel
                std::map<std::string, Channel>::iterator it = _channels.find(target);
                
                if (it != _channels.end()) { // Channel exists
                    Channel& channel = it->second;

                    // CRUCIAL CHECK: Is sender a channel member?
                    if (channel.isClientInChannel(client_fd)) {
                        // Yes, they are a member. Can send the message.
                        std::string full_message = ":" + client.getNickname() + " PRIVMSG " + target + " :" + message_text + "\r\n";
                        channel.broadcastMessage(full_message, client_fd);
                    } else {
                        // No, they are not a member (was kicked or never joined).
                        // Send error: ERR_CANNOTSENDTOCHAN (404)
                        std::string error_msg = ":irc.42.fr 404 " + client.getNickname() + " " + target + " :Cannot send to channel\r\n";
                        send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                        std::cout << "WARNING: " << client.getNickname() << " (non-member) tried to send msg to " << target << std::endl;
                    }
                } else {
                    // Channel does not exist.
                    // Send error: ERR_NOSUCHCHANNEL (403)
                    std::string error_msg = ":irc.42.fr 403 " + client.getNickname() + " " + target + " :No such channel\r\n";
                    send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                    std::cout << "WARNING: Client " << client.getNickname() << " tried to send msg to non-existent channel " << target << std::endl;
                }
            } else { // Private Message to a User
                int target_fd = getClientFdByNickname(target);

                if (target_fd != -1) {
                    // User found, send the message.
                    std::string private_message = ":" + client.getNickname() + " PRIVMSG " + target + " :" + message_text + "\r\n";
                    send(target_fd, private_message.c_str(), private_message.length(), 0);
                } else {
                    // User not found.
                    // Send error: ERR_NOSUCHNICK (401)
                    std::string error_msg = ":irc.42.fr 401 " + client.getNickname() + " " + target + " :No such nick/channel\r\n";
                    send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                    std::cout << "WARNING: " << client.getNickname() << " tried to send msg to non-existent user: " << target << std::endl;
                }
            }
        }
        else if (command == "KICK") {
            // Parse: KICK <#channel> <user> [:<reason>]
            size_t first_space = params.find(' ');
            if (first_space == std::string::npos) { /* Enviar erro: ERR_NEEDMOREPARAMS */ return; }
            
            std::string channel_name = params.substr(0, first_space);
            std::string user_to_kick_nick = params.substr(first_space + 1);
            std::string reason = "Kicked by operator"; // Razão padrão
            
            size_t colon_pos = user_to_kick_nick.find(':');
            if (colon_pos != std::string::npos) {
                reason = user_to_kick_nick.substr(colon_pos + 1);
                user_to_kick_nick = user_to_kick_nick.substr(0, colon_pos - 1);
            }

            // 1. Does channel exist?
            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) { 
                /* Enviar erro: ERR_NOSUCHCHANNEL */ 
                sendReply(client_fd, 403, client_nick, channel_name + " :No such channel"); // ERR_NOSUCHCHANNEL
                return; 
            }
            
            Channel& channel = it->second;

            // 2. Is the person trying to kick an operator?
            if (!channel.isOperator(client_fd)) { 
                /* Enviar erro: ERR_CHANOPRIVSNEEDED */
                sendReply(client_fd, 482, client_nick, channel_name + " :You're not channel operator"); // ERR_CHANOPRIVSNEEDED 
                return; }

            // 3. Does the user to be kicked exist on the server?
            int user_to_kick_fd = getClientFdByNickname(user_to_kick_nick);
            if (user_to_kick_fd == -1) { 
                /* Enviar erro: ERR_NOSUCHNICK */ 
                sendReply(client_fd, 401, client_nick, user_to_kick_nick + " :No such nick/channel"); // ERR_NOSUCHNICK
                return; 
            }
            
            // 4. Is the user to be kicked in the channel?
            const std::vector<int>& clients_in_channel = channel.getClients();
            bool user_is_in_channel = false;
            for (size_t i = 0; i < clients_in_channel.size(); ++i) {
                if (clients_in_channel[i] == user_to_kick_fd) {
                    user_is_in_channel = true;
                    break;
                }
            }
            if (!user_is_in_channel) { 
                /* Enviar erro: ERR_USERNOTINCHANNEL */ 
                sendReply(client_fd, 441, client_nick, user_to_kick_nick + " " + channel_name + " :They aren't on that channel"); // ERR_USERNOTINCHANNEL
                return; 
            }
            
            // Everything OK, let's kick!
            // 1. Build KICK message
            std::string kick_msg = ":" + client.getNickname() + " KICK " + channel_name + " " + user_to_kick_nick + " :" + reason + "\r\n";

            // 2. Send message to ALL in channel (including who was kicked)
            const std::vector<int>& all_clients = channel.getClients();
            for (size_t i = 0; i < all_clients.size(); ++i) {
                send(all_clients[i], kick_msg.c_str(), kick_msg.length(), 0);
            }
            
            // 3. Remove user from channel
            channel.removeClient(user_to_kick_fd);
            std::cout << "INFO: " << client.getNickname() << " kicked " << user_to_kick_nick << " from channel " << channel_name << std::endl;
        }
        else if (command == "PART") {
            if (params.empty()) { 
                /* Enviar erro: ERR_NEEDMOREPARAMS */
                sendReply(client_fd, 461, client_nick, "PART :Not enough parameters"); // ERR_NEEDMOREPARAMS 
                return; 
            }
            
            std::string channel_name = params.substr(0, params.find(' '));
            std::string part_message = client.getNickname(); // Default message

            // Does channel exist?
            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) { 
                /* Enviar erro: ERR_NOSUCHCHANNEL */ 
                sendReply(client_fd, 403, client_nick, channel_name + " :No such channel"); // ERR_NOSUCHCHANNEL
                return; 
            }
            
            Channel& channel = it->second;

            // Is client in channel?
            if (!channel.isClientInChannel(client_fd)) { 
                /* Enviar erro: ERR_NOTONCHANNEL */ 
                sendReply(client_fd, 442, client_nick, channel_name + " :You're not on that channel"); // ERR_NOTONCHANNEL
                return; 
            }

            // Everything OK, let's process the exit.
            // 1. Build PART message
            std::string part_notification = ":" + client.getNickname() + " PART " + channel_name + " :" + part_message + "\r\n";
            
            // 2. Send notification to ALL channel members
            const std::vector<int>& clients_in_channel = channel.getClients();
            for (size_t i = 0; i < clients_in_channel.size(); ++i) {
                send(clients_in_channel[i], part_notification.c_str(), part_notification.length(), 0);
            }
            
            // 3. Remove client from channel
            channel.removeClient(client_fd);
            std::cout << "INFO: " << client.getNickname() << " left channel " << channel_name << std::endl;
        }
        else if (command == "TOPIC") {
            if (params.empty()) {
                // Send error: ERR_NEEDMOREPARAMS (461)
                return;
            }

            size_t colon_pos = params.find(" :"); // Look for " :" to separate topic
            std::string channel_name = (colon_pos == std::string::npos) ? params : params.substr(0, colon_pos);
            
            // Remove whitespace from end of channel_name, if any
            size_t last_char = channel_name.find_last_not_of(" \t\r\n");
            if (last_char != std::string::npos) {
                channel_name.erase(last_char + 1);
            }

            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) { 
                // Send error: ERR_NOSUCHCHANNEL (403)
                return; 
            }
            
            Channel& channel = it->second;

            if (!channel.isClientInChannel(client_fd)) { 
                // Send error: ERR_NOTONCHANNEL (442)
                return; 
            }

            // If there's no ':', user wants to SEE the topic.
            if (colon_pos == std::string::npos) {
                // LOGIC TO VIEW TOPIC
                if (channel.getTopic().empty()) {
                    // Send RPL_NOTOPIC (331)
                    std::string no_topic_msg = ":irc.42.fr 331 " + client.getNickname() + " " + channel_name + " :No topic is set\r\n";
                    send(client_fd, no_topic_msg.c_str(), no_topic_msg.length(), 0);
                } else {
                    // Send RPL_TOPIC (332)
                    std::string topic_msg = ":irc.42.fr 332 " + client.getNickname() + " " + channel_name + " :" + channel.getTopic() + "\r\n";
                    send(client_fd, topic_msg.c_str(), topic_msg.length(), 0);
                }
            } else {
                // LOGIC TO SET TOPIC
                // Check if topic is restricted AND user is NOT an operator
                if (channel.isTopicRestricted() && !channel.isOperator(client_fd)) {
                    // Send error: ERR_CHANOPRIVSNEEDED (482)
                    std::string error_msg = ":irc.42.fr 482 " + client.getNickname() + " " + channel_name + " :You're not channel operator\r\n";
                    send(client_fd, error_msg.c_str(), error_msg.length(), 0);
                    return;
                }
                
                std::string new_topic = params.substr(colon_pos + 2); // +2 to skip " :"
                channel.setTopic(new_topic);

                // Notify everyone in channel about new topic
                std::string topic_notification = ":" + client.getNickname() + " TOPIC " + channel_name + " :" + new_topic + "\r\n";
                
                const std::vector<int>& clients_in_channel = channel.getClients();
                for (size_t i = 0; i < clients_in_channel.size(); ++i) {
                    send(clients_in_channel[i], topic_notification.c_str(), topic_notification.length(), 0);
                }
            }
        }
        else if (command == "INVITE") {
            // Parse: INVITE <nickname> <#channel>
            size_t first_space = params.find(' ');
            if (first_space == std::string::npos) { /* ERR_NEEDMOREPARAMS */ return; }
            std::string nick_to_invite = params.substr(0, first_space);
            std::string channel_name = params.substr(first_space + 1);

            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) { /* ERR_NOSUCHCHANNEL */ return; }
            Channel& channel = it->second;

            if (!channel.isClientInChannel(client_fd)) { /* ERR_NOTONCHANNEL */ return; }
            if (channel.isInviteOnly() && !channel.isOperator(client_fd)) { /* ERR_CHANOPRIVSNEEDED */ return; }
            
            int target_fd = getClientFdByNickname(nick_to_invite);
            if (target_fd == -1) { /* ERR_NOSUCHNICK */ return; }

            channel.addInvite(target_fd);
            // Notify invite author and send invite to target
            std::string invite_msg = ":" + client.getNickname() + " INVITE " + nick_to_invite + " :" + channel_name + "\r\n";
            send(target_fd, invite_msg.c_str(), invite_msg.length(), 0);
        }
        else if (command == "MODE") {
            // Parse: MODE <#channel> <+|-><modes> [parameters]
            // This parse logic can be complex. Let's simplify.
            size_t first_space = params.find(' ');
            if (first_space == std::string::npos) { /* ERR_NEEDMOREPARAMS */ return; }
            std::string channel_name = params.substr(0, first_space);
            std::string rest = params.substr(first_space + 1);
            
            size_t second_space = rest.find(' ');
            std::string mode_str = (second_space == std::string::npos) ? rest : rest.substr(0, second_space);
            std::string mode_param = (second_space == std::string::npos) ? "" : rest.substr(second_space + 1);

            std::map<std::string, Channel>::iterator it = _channels.find(channel_name);
            if (it == _channels.end()) { 
                /* ERR_NOSUCHCHANNEL */ 
                sendReply(client_fd, 403, client_nick, channel_name + " :No such channel");
                return; 
            }
            Channel& channel = it->second;

            if (!channel.isOperator(client_fd)) { 
                /* ERR_CHANOPRIVSNEEDED */ 
                sendReply(client_fd, 482, client_nick, channel_name + " :You're not channel operator");
                return; 
            }
            
            bool add_mode = (mode_str[0] == '+');
            char mode_char = mode_str[1];

            switch(mode_char) {
                case 'i':
                    channel.setInviteOnly(add_mode);
                    break;
                case 't':
                    channel.setTopicRestricted(add_mode);
                    break;
                case 'k':
                    if (add_mode) {
                        if (mode_param.empty()) { 
                            /* ERR_NEEDMOREPARAMS */ 
                            sendReply(client_fd, 461, client_nick, "MODE +k :Not enough parameters");
                            return; 
                        }
                        channel.setKey(mode_param);
                    } else {
                        channel.removeKey();
                    }
                    break;
                case 'o': {
                    if (mode_param.empty()) { /* ERR_NEEDMOREPARAMS */ return; }
                    int target_fd = getClientFdByNickname(mode_param);
                    if (target_fd == -1) { 
                        /* ERR_NOSUCHNICK */ 
                        sendReply(client_fd, 401, client_nick, mode_param + " :No such nick/channel");
                        return; 
                    }
                    if (add_mode) channel.addOperator(target_fd);
                    else channel.removeOperator(target_fd);
                    break;
                }
                case 'l':
                    if (add_mode) {
                        if (mode_param.empty()) { /* ERR_NEEDMOREPARAMS */ return; }
                        int limit = std::atoi(mode_param.c_str());
                        channel.setUserLimit(limit > 0 ? limit : -1);
                    } else {
                        channel.setUserLimit(-1); // Remove o limite
                    }
                    break;
                default:
                    sendReply(client_fd, 472, client_nick, std::string(1, mode_char) + " :is unknown mode char to me for " + channel_name); // ERR_UNKNOWNMODE
                    break;
            }
            // Notify everyone in channel about mode change
            std::string mode_notification = ":" + client.getNickname() + " MODE " + channel_name + " " + mode_str + " " + mode_param + "\r\n";
            const std::vector<int>& clients_in_channel = channel.getClients();
            for (size_t i = 0; i < clients_in_channel.size(); ++i) {
                send(clients_in_channel[i], mode_notification.c_str(), mode_notification.length(), 0);
            }
        }

        // Add other commands (PART, etc.) here as 'else if'
    }
    // Logic for UNREGISTERED clients that send invalid commands
    else {
        //std::cout << "AVISO: Comando '" << command << "' recebido de cliente não registrado (FD: " << client_fd << ")." << std::endl;
        sendReply(client_fd, 451, client_nick, ":You have not registered"); // ERR_NOTREGISTERED
    }
}

void Server::removeClient(int client_fd) {

    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end(); ++it) {
        it->second.removeClient(client_fd);
    }

    close(client_fd);
    _clients.erase(client_fd);

    for (size_t i = 0; i < _fds.size(); ++i)
    {
        if (_fds[i].fd == client_fd)
        {
            _fds.erase(_fds.begin() + i);
            break;
        }
    }
}


bool Server::isNicknameInUse(const std::string& nick) {
    // Iterate over client map
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.getNickname() == nick) {
            return true; // Found a client with the same nickname
        }
    }
    return false; // Nickname is free
}


void Server::checkRegistration(int client_fd) {
    Client& client = _clients[client_fd];

    // Is client already registered? Then do nothing.
    if (client.isRegistered()) {
        return;
    }

    // To be registered, client needs to have provided password,
    // a non-empty nickname and a non-empty username.
    if (client.isPasswordCorrect() && !client.getNickname().empty() && !client.getUsername().empty()) {
        client.setRegistered(true);

        // --- SEND WELCOME MESSAGE (RPL_WELCOME) ---
        // This is a crucial part of the IRC protocol.
        // Format is: ":<server> 001 <nick> :<message>"
        std::string welcome_msg = ":irc.42.fr 001 " + client.getNickname() + " :Welcome to the Internet Relay Network " + client.getNickname() + "\r\n";
        
        send(client_fd, welcome_msg.c_str(), welcome_msg.length(), 0);
        
        std::cout << "INFO: Client " << client.getNickname() << " (FD: " << client_fd << ") was registered successfully." << std::endl;
    }
}

int Server::getClientFdByNickname(const std::string& nick) {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.getNickname() == nick) {
            return it->first; // Return client's file descriptor (fd)
        }
    }
    return -1; // Return -1 if not found
}


// Function to format and send numeric replies to client
void Server::sendReply(int client_fd, int code, const std::string& target, const std::string& message) {
    std::stringstream ss;
    ss << ":irc.42.fr " << code << " " << target << " " << message << "\r\n";
    std::string reply = ss.str();
    
    // Show in server log for debugging
    std::cout << ">> Sending to FD " << client_fd << ": " << reply;

    send(client_fd, reply.c_str(), reply.length(), 0);
}
