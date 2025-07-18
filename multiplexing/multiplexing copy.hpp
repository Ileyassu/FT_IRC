#ifndef MULTIPLEXING_HPP
#define MULTIPLEXING_HPP

#include "server.hpp"
#include "../IRC_msg_protocol/Client.hpp"
#include "../IRC_msg_protocol/Channel.hpp"
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <string>
#include <ctime>

#define MAX_EVENTS 1024

class Multiplexer
{
private:
	int epoll_fd;
	Server serverFd;
	epoll_event event, events[MAX_EVENTS];
	std::map<int, Client> clients;
	std::map<std::string, int> nicknameToFd;
	std::map<std::string, Channel *> channels;
	std::string serverPassword;
	time_t serverCreationTime;
	std::string serverpassword;

public:
	Multiplexer();
	Multiplexer(Server &serverSocket);
	Server &getServer();
	~Multiplexer();
	const int &getEpollFd();
	epoll_event *getEpollEvents();
	void setEpollInstance();
	void event_loop();
	void handleRead(int fd);
	void handleWrite(int fd);
	void handleError(int fd);
	void removeClient(int fd);
	void addClient(int fd , const sockaddr_in &clientAddr);
	Client findfd(int fd);
std::string getClientFullIdentity(int fd);

	bool isNicknameInUse(const std::string &nickname);
	void setClientNickname(int fd, const std::string &nickname);
	void setClientUser(int fd, const std::string &username, const std::string &realname);
	void setClientPassword(int fd, const std::string &password);
	bool isClientRegistered(int fd);
	std::string getClientNickname(int fd);
	std::string getClientUsername(int fd);
	void sendToClient(int fd, const std::string &message);
	void resetClientTimeout(int fd);

	void joinChannel(int fd, const std::string &channelName);
	void joinChannel(int fd, const std::string &channelName, const std::string &key);
	void partChannel(int fd, const std::string &channelName, const std::string &partMsg);
	void sendPrivateMessage(int fd, const std::string &target, const std::string &message);
	void sendNotice(int fd, const std::string &target, const std::string &message);
	void broadcastToClientChannels(int fd, const std::string &message);
	void broadcastToChannel(const std::string &channelName, const std::string &message);
	void broadcastToChannel(const std::string &channelName, const std::string &message, int excludeFd);

	std::string getChannelTopic(const std::string &channelName);
	bool setChannelTopic(int fd, const std::string &channelName, const std::string &topic);
	std::string getChannelModes(const std::string &channelName);
	void setChannelMode(int fd, const std::string &channelName, const std::string &modeString, const std::vector<std::string> &params);

	void kickFromChannel(int fd, const std::string &channelName, const std::string &target, const std::string &reason);
	void inviteToChannel(int fd, const std::string &target, const std::string &channelName);
	bool isChannelOperator(int fd, const std::string &channelName);

	void sendChannelNames(int fd, const std::string &channelName);
	void sendAllChannelNames(int fd);
	void sendChannelList(int fd, const std::string &channels);

	std::string getServerCreationTime();

	int findClientByNickname(const std::string &nickname);
	bool isClientInChannel(int fd, const std::string &channelName);
};

#endif