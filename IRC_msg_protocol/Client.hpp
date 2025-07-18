#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "Message.hpp"
enum class ClientState {
    UNREGISTERED,    // Just connected
    NICK_SET,        // Has nickname
    USER_SET,        // Has user info
    REGISTERED,       // Fully registered (can join channels)
	AUTH
};

// TODO refactor had l9lawi	

class Client
{
private:
	int socket_fd;
	sockaddr_in address;
	std::string nickname;
	std::string username;
	std::string realname;
	std::string messageBuffer;
	// Message messageHandler();
	
	bool isRegistered;
	bool isAuthenticated;

public:
	// Constructors and destructor
	Client();
	Client(int fd, const sockaddr_in &addr);
	Client(const Client &other);
	~Client();

	// Methods
	void appendToBuffer(const std::string &data);
	void handleMessage(std::string message);
	int getSocketFd() const;
	sockaddr_in getAddress() const;
	std::string getNickname() const;
	std::string getUsername() const;
	std::string getRealname() const;
	void setNickname(const std::string &nick);
	void setUsername(const std::string &user);
	void setRealname(const std::string &name);
	void setAuthenticated(bool auth);
	void setRegistered(bool reg);
	bool isClientRegistered() const;
	bool isClientAuthenticated() const;
	std::string getBuffer(void) const;


};

#endif // CLIENT_HPP
