#ifndef CLINENT_HPP
#define CLINENT_HPP
#include <iostream>


class Client {

	private:
		int         _fd;
		std::string _nickname;
		std::string _username;
		bool        _passwordCorrect;
		std::string _buffer; // Store partial data
		bool        _isRegistered; // Has client completed PASS, NICK and USER?
	
	public:
		Client();
		Client(int fd);
		~Client();

		int getFd() const;
		bool isPasswordCorrect() const;
		void setPasswordCorrect(bool val);
		// Buffer to store received data that doesn't form a complete command yet
		std::string& getBuffer();
		const std::string& getNickname() const;
		const std::string& getUsername() const;
		bool isRegistered() const;
		void setNickname(const std::string& nick);
		void setUsername(const std::string& user);
		void setRegistered(bool val);

		

};
#endif