#include "Channel.hpp"
#include <sys/socket.h> // Para a função send


//Channel::Channel(const std::string& name) : _name(name) {}

Channel::Channel(const std::string& name) : 
    _name(name), 
    _topic("No topic is set."),
    _isInviteOnly(false),       // By default, channel is public
    _isTopicRestricted(true),   // By default, only ops change topic
    _userLimit(-1)              // By default, no user limit
{}

Channel::~Channel() {}

const std::string& Channel::getName() const {
    return _name;
}

void Channel::addClient(int client_fd) {
    // Check if client is already in channel
    if (isClientInChannel(client_fd)) {
        return; // Client already in channel, don't add again
    }
    
    // If channel is empty, first to enter becomes operator
    if (_clients_fds.empty()) {
        addOperator(client_fd);
    }
    _clients_fds.push_back(client_fd);
}

/*void Channel::addClient(int client_fd) {
    _clients_fds.push_back(client_fd);
}*/

void Channel::removeClient(int client_fd) {
    // Remove client from operators list, if they are one
    removeOperator(client_fd);
    
    // Remove client from general list
    for (size_t i = 0; i < _clients_fds.size(); ++i) {
        if (_clients_fds[i] == client_fd) {
            _clients_fds.erase(_clients_fds.begin() + i);
            break;
        }
    }
}

/*void Channel::removeClient(int client_fd) {
    for (size_t i = 0; i < _clients_fds.size(); ++i) {
        if (_clients_fds[i] == client_fd) {
            _clients_fds.erase(_clients_fds.begin() + i);
            break;
        }
    }
}*/

// Send a message to ALL clients in channel, EXCEPT the sender
void Channel::broadcastMessage(const std::string& message, int sender_fd) {
    for (size_t i = 0; i < _clients_fds.size(); ++i) {
        if (_clients_fds[i] != sender_fd) {
            send(_clients_fds[i], message.c_str(), message.length(), 0);
        }
    }
}

const std::vector<int>& Channel::getClients() const {
    return _clients_fds;
}



void Channel::addOperator(int client_fd) {
    _operators_fds.push_back(client_fd);
}

void Channel::removeOperator(int client_fd) {
    for (size_t i = 0; i < _operators_fds.size(); ++i) {
        if (_operators_fds[i] == client_fd) {
            _operators_fds.erase(_operators_fds.begin() + i);
            break;
        }
    }
}

bool Channel::isOperator(int client_fd) const {
    for (size_t i = 0; i < _operators_fds.size(); ++i) {
        if (_operators_fds[i] == client_fd) {
            return true;
        }
    }
    return false;
}

bool Channel::isClientInChannel(int client_fd) const {
    for (size_t i = 0; i < _clients_fds.size(); ++i) {
        if (_clients_fds[i] == client_fd) {
            return true; // Found client in members list
        }
    }
    return false; // Not found
}




const std::string& Channel::getTopic() const { return _topic; }
void Channel::setTopic(const std::string& topic) { _topic = topic; }





//MODES

void Channel::setInviteOnly(bool val) { _isInviteOnly = val; }
void Channel::setTopicRestricted(bool val) { _isTopicRestricted = val; }
void Channel::setKey(const std::string& key) { _key = key; }
void Channel::removeKey() { _key.clear(); }
void Channel::setUserLimit(int limit) { _userLimit = limit; }

bool Channel::isInviteOnly() const { return _isInviteOnly; }
bool Channel::isTopicRestricted() const { return _isTopicRestricted; }
bool Channel::hasKey() const { return !_key.empty(); }
const std::string& Channel::getKey() const { return _key; }
int  Channel::getUserLimit() const { return _userLimit; }

void Channel::addInvite(int client_fd) {
    if (!isInvited(client_fd)) {
        _invited_fds.push_back(client_fd);
    }
}

bool Channel::isInvited(int client_fd) const {
    for (size_t i = 0; i < _invited_fds.size(); ++i) {
        if (_invited_fds[i] == client_fd) return true;
    }
    return false;
}