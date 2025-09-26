#include "Client.hpp"

    Client::Client() : _fd(-1), _passwordCorrect(false), _isRegistered(false) {}

    Client::Client(int fd) : _fd(fd), _passwordCorrect(false), _isRegistered(false) {}

    Client::~Client() {}

    int Client::getFd() const 
    { 
        return _fd; 
    }

    bool Client::isPasswordCorrect() const 
    { 
        return _passwordCorrect; 
    }

    void Client::setPasswordCorrect(bool val) 
    { 
        _passwordCorrect = val; 
    }

    // Buffer to store received data that doesn't form a complete command yet
    std::string& Client::getBuffer() 
    { 
        return _buffer; 
    }

    const std::string& Client::getNickname() const 
    { 
        return _nickname; 
    }
    
    const std::string& Client::getUsername() const 
    { 
        return _username; 
    }
    bool Client::isRegistered() const 
    { 
        return _isRegistered; 
    }

    void Client::setNickname(const std::string& nick) 
    { 
        _nickname = nick; 
    }
    
    void Client::setUsername(const std::string& user) 
    { 
        _username = user; 
    }

    void Client::setRegistered(bool val) 
    { 
        _isRegistered = val; 
    }