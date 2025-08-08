#ifndef MULTIPLEXING_HPP
#define MULTIPLEXING_HPP

#include "server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/epoll.h>
#include <exception>
#include "../IRC_msg_protocol/Client.hpp"
#include "../IRC_msg_protocol/Channel.hpp"
#include <sstream>
#include <map>
class Client;
class Channel;
#include <set>
#define MAX_EVENTS 1024
class Multiplexer
{
private:
	int epoll_fd;
	Server serverFd;
	std::set<Client *> clients;
	std::map<std::string, Channel *> channels;
	struct epoll_event event, events[MAX_EVENTS];
	std::string serverPassword;

public:
	Multiplexer();
	Multiplexer(int port, const std::string &password);
	Multiplexer(Server &serverSocket);
	Server &getServer();
	~Multiplexer();
	const int &getEpollFd();
	epoll_event *getEpollEvents();
	void setEpollInstance();
	void event_loop();
	void handleRead(int fd);

	void handleClientMessage(Client *client);
	void handleIRCCommand(Client *client, std::string msg);

	void handleWrite(int fd);
	void handleError(int fd);
	Client *findFd(int fd);
	Client *findClientByNickname(const std::string &nickname);

	std::string extractCommand(std::string rawMessage);
	void processClientMessages(Client *client);
	void removeClient(int fd);
	void handleNewConnection();

	Channel *getOrCreateChannel(const std::string &channelName);
	Channel *getChannel(const std::string &channelName);
	void removeChannel(const std::string &channelName);
	void sendToClient(Client *client, const std::string &message);
	void sendWelcomeMessages(Client *client);

	bool checkPassword(const std::string &password);

	bool isClientInChannel(int fd, const std::string &channelName);
	bool isChannelOperator(int fd, const std::string &channelName);
	void kickFromChannel(int fd, const std::string &channelName, const std::string &target, const std::string &reason);
	void inviteToChannel(int fd, const std::string &target, const std::string &channelName);
	bool setChannelTopic(int fd, const std::string &channelName, const std::string &topic);
	std::string getChannelTopic(const std::string &channelName);
	std::string getChannelModes(const std::string &channelName);
	void setChannelMode(int fd, const std::string &channelName, const std::string &modeString, const std::vector<std::string> &params);

	static Multiplexer *instance;
	static Multiplexer *getInstance();
	void cleanupEmptyChannels();
	void cleanUpServer(void);
};

#endif