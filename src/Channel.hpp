#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <string>
#include <vector>
#include "Client.hpp"

class Channel {
    public:
        Channel(const std::string& name);
        ~Channel();

        const std::string& getName() const;
        void addClient(int client_fd);
        void removeClient(int client_fd);
        void broadcastMessage(const std::string& message, int sender_fd);
        const std::vector<int>& getClients() const;

        void addOperator(int client_fd);
        void removeOperator(int client_fd);
        bool isOperator(int client_fd) const;

        bool isClientInChannel(int client_fd) const;

        const std::string& getTopic() const;
        void setTopic(const std::string& topic);

        void setInviteOnly(bool val);
        void setTopicRestricted(bool val);
        void setKey(const std::string& key);
        void removeKey();
        void setUserLimit(int limit);
        
        bool isInviteOnly() const;
        bool isTopicRestricted() const;
        bool hasKey() const;
        const std::string& getKey() const;
        int  getUserLimit() const;
        
        void addInvite(int client_fd);
        bool isInvited(int client_fd) const;

    private:
        std::string         _name;
        std::vector<int>    _clients_fds; // Store file descriptors of clients in channel
        // Future: topic, channel modes, operators list, etc.

        std::vector<int>    _operators_fds; // Store operator fds

        std::string         _topic;


        bool                _isInviteOnly;
        bool                _isTopicRestricted;
        std::string         _key;
        int                 _userLimit;
        std::vector<int>    _invited_fds; // List of who was invited (for +i mode)
};

#endif