#include "Client.hpp"

Client::Client(int fd, const sockaddr_in &addr)
	: socket_fd(fd), address(addr)
{
	std::cout << "Client created with fd: " << socket_fd << std::endl;
	isAuthenticated = false;
	isRegistered = false;
}
Client::Client() : socket_fd(-1)
{
	memset(&address, 0, sizeof(address));
}
int Client::getSocketFd() const
{
	return socket_fd;
}
void Client::appendToBuffer(const std::string &data)
{
	messageBuffer += data;
}
sockaddr_in Client::getAddress() const
{
	return address;
}
std::string Client::getNickname() const
{
	return nickname;
}
std::string Client::getUsername() const
{
	return username;
}
std::string Client::getRealname() const
{
	return realname;
}
void Client::setNickname(const std::string &nick)
{
	nickname = nick;
}
void Client::setUsername(const std::string &user)
{
	username = user;
}
void Client::setRealname(const std::string &name)
{
	realname = name;
}
Client::~Client()
{
	if (socket_fd != -1)
	{
		close(socket_fd);
	}
}
Client::Client(const Client &other)
	: socket_fd(other.socket_fd), address(other.address),
	  nickname(other.nickname), username(other.username), realname(other.realname) {}

void Client::setAuthenticated(bool auth)
{
	isAuthenticated = auth;
}
bool Client::isClientRegistered() const
{
	return !nickname.empty() && !username.empty();
}
bool Client::isClientAuthenticated() const
{
	return isAuthenticated;
}
void Client::setRegistered(bool reg)
{
	isRegistered = reg;
}

void Client::handleMessage(std::string message)
{
	messageHandler.parseMessage(message);
	// messageHandler(message);
}

std::string Client::getBuffer() const { return messageBuffer };