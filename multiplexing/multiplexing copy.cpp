#include "multiplexing.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "../IRC_msg_protocol/Message.hpp"
#include "../IRC_msg_protocol/Client.hpp"

#include <fcntl.h>
#include <arpa/inet.h>

Multiplexer::Multiplexer() : serverFd(), serverCreationTime(time(NULL))
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
};

Multiplexer::Multiplexer(Server &serverSocket) : epoll_fd(epoll_create1(0)), serverFd(serverSocket), serverCreationTime(time(NULL))
{
	this->serverPassword = serverSocket.getPassword();

	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
}

void Multiplexer::setEpollInstance()
{
	event.events = EPOLLIN | EPOLLOUT;
	event.data.fd = serverFd.getServerFd();
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverFd.getServerFd(), &event) == -1)
		throw std::runtime_error("Error : error while monitoring fd (epoll_ctl)");
	if (listen(serverFd.getServerFd(), 128) == -1)
		throw std::runtime_error("Error : passive socket error, while listening");
	if (fcntl(serverFd.getServerFd(), F_SETFL, O_NONBLOCK))
		throw std::runtime_error("Error : fcntl error");
}

Multiplexer::~Multiplexer()
{
	close(epoll_fd);
}

const int &Multiplexer::getEpollFd()
{
	return epoll_fd;
}

epoll_event *Multiplexer::getEpollEvents()
{
	return events;
}

Server &Multiplexer::getServer()
{
	return serverFd;
}

void Multiplexer::handleRead(int fd){
    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer) -1, 0);
    if(bytesRead <= 0){
        if(bytesRead == 0)
            std::cout << "Client disconnected\n";
        else
            std::cerr << "Can't connect to the client\n";
        if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL))
            throw std::runtime_error("Error while removing client from epoll");
        close(fd);
    }
    else
        std::cout << "Client : " << buffer << std::endl;

}

void Multiplexer::addClient(int fd, const sockaddr_in& clientAddr)
{

    if (clients.find(fd) != clients.end()) {
        std::cout << "WARNING: Client " << fd << " already exists, removing stale entry\n";
        clients.erase(fd);
    }
    
    Client client(fd, clientAddr);
    clients.insert(std::pair<int, Client>(fd, client));
    std::cout << "Added client " << fd << " to client list\n";
}

void Multiplexer::removeClient(int fd)
{
    std::cout << "=== REMOVING CLIENT " << fd << " ===" << std::endl;
    
    auto it = clients.find(fd);
    if (it != clients.end()) {
        clients.erase(it);
        std::cout << "Removed client " << fd << " from client map" << std::endl;
    } else {
        std::cout << "WARNING: Client " << fd << " not found in client map" << std::endl;
    }
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        std::cerr << "Error removing fd " << fd << " from epoll: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Removed fd " << fd << " from epoll" << std::endl;
    }
    
    if (close(fd) == -1) {
        std::cerr << "Error closing fd " << fd << ": " << strerror(errno) << std::endl;
    } else {
        std::cout << "Closed fd " << fd << std::endl;
    }
    
    std::cout << "=== CLIENT " << fd << " CLEANUP COMPLETE ===" << std::endl;
}

void Multiplexer::handleError(int fd)
{
	std::cerr << "Client error or hangup (fd: " << fd << ")" << std::endl;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL))
		throw std::runtime_error("Error while removing client from epoll");
	close(fd);
}

void Multiplexer::event_loop()
{
    while(1){
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(nfds == -1){
            if(errno == EINTR)
                continue;
            throw std::runtime_error("Error : Epoll wait failed");
        }
        for(int i = 0; i < nfds; i++){
            if(events[i].data.fd == serverFd.getServerFd()){
                sockaddr_in clientAddr;
                socklen_t sockAddressLen = sizeof(serverFd.getServerAddress());
                int clientFd = accept(serverFd.getServerFd(), (sockaddr *)&clientAddr, &sockAddressLen);
				std::cout << "New client want to connect " << clientFd << std::endl; 
                if(clientFd == -1){
                    if(errno == EAGAIN || errno == EWOULDBLOCK)
                        break;
                    std::cerr << "Error accepting client\n";
                }
                fcntl(clientFd, F_SETFL, O_NONBLOCK);
                struct epoll_event clientEvent;
                clientEvent.events = EPOLLIN;
                clientEvent.data.fd = clientFd;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1){
                    throw std::runtime_error("Error while adding client to epoll");
                }
            }

            else {
                int clientFd = events[i].data.fd;

                if (events[i].events & EPOLLIN) {
                    handleRead(clientFd);
                }

                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    std::cout << "Client error or hangup (fd: " << clientFd << ")" << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);
                }
            }
        }
    }
}

bool Multiplexer::isNicknameInUse(const std::string &nickname)
{
	for (std::map<int, Client>::const_iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if (it->second.getNickname() == nickname)
		{
			return true;
		}
	}
	return false;
}

void Multiplexer::setClientNickname(int fd, const std::string &nickname)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{

		std::string oldNick = it->second.getNickname();
		if (!oldNick.empty())
		{
			nicknameToFd.erase(oldNick);
		}

		it->second.setNickname(nickname);
		nicknameToFd[nickname] = fd;
	}
}

void Multiplexer::setClientUser(int fd, const std::string &username, const std::string &realname)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		it->second.setUsername(username);
		it->second.setRealname(realname);
	}
}

void Multiplexer::setClientPassword(int fd, const std::string &password)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
	{
		std::cerr << "Client " << fd << " not found" << std::endl;
		return;
	}

	std::cout << "Password authentication for client " << fd << std::endl;

	if (password == serverPassword)
	{
		it->second.setAuthenticated(true);
		std::string nickname = it->second.getNickname();
		if (!nickname.empty())
		{
			sendToClient(fd, ":ft_irc_server 001 " + nickname + " :Welcome to the IRC server!\r\n");
		}
		else
		{
			sendToClient(fd, ":ft_irc_server 001 * :Welcome to the IRC server!\r\n");
		}
	}
	else
	{
		sendToClient(fd, ":ft_irc_server 464 :Password incorrect\r\n");
	}
}

bool Multiplexer::isClientRegistered(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it == clients.end())
	{
		return false;
	}

	if (it->second.getNickname().empty() || it->second.getUsername().empty())
	{
		return false;
	}

	return true;
}

std::string Multiplexer::getClientNickname(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		return it->second.getNickname();
	}
	return "";
}

std::string Multiplexer::getClientUsername(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		return it->second.getUsername();
	}
	return "";
}

void Multiplexer::sendToClient(int fd, const std::string &message)
{

	send(fd, message.c_str(), message.length(), 0);
}

void Multiplexer::resetClientTimeout(int fd)
{

	(void)fd;
	std::cout << "Reset timeout for client " << fd << std::endl;
}

Client Multiplexer::findfd(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		return it->second;
	}
	throw std::runtime_error("Client not found");
}

void Multiplexer::joinChannel(int fd, const std::string &channelName)
{
	joinChannel(fd, channelName, "");
}

void Multiplexer::joinChannel(int fd, const std::string &channelName, const std::string &key)
{
	std::string nickname = getClientNickname(fd);
	if (nickname.empty())
	{
		sendToClient(fd, "451 :You have not registered\r\n");
		return;
	}

	if (isClientInChannel(fd, channelName))
	{
		return;
	}

	Channel *channel = nullptr;
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it == channels.end())
	{

		channel = new Channel(channelName, key);
		channels[channelName] = channel;

		channel->addOperator(fd);
	}
	else
	{
		channel = it->second;
	}

	channel->addMember(fd, nullptr);

	std::string joinMsg = ":" + nickname + " JOIN " + channelName + "\r\n";
	broadcastToChannel(channelName, joinMsg);

	std::string topic = getChannelTopic(channelName);
	if (!topic.empty())
	{
		sendToClient(fd, ":ft_irc_server 332 " + nickname + " " + channelName + " :" + topic + "\r\n");
	}
	else
	{
		sendToClient(fd, ":ft_irc_server 331 " + nickname + " " + channelName + " :No topic is set\r\n");
	}

	sendChannelNames(fd, channelName);

	std::cout << "Client " << nickname << " joined channel " << channelName << std::endl;
}

void Multiplexer::partChannel(int fd, const std::string &channelName, const std::string &partMsg)
{
	std::string nickname = getClientNickname(fd);
	if (nickname.empty())
		return;

	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{

		std::string partMessage = ":" + nickname + " PART " + channelName;
		if (!partMsg.empty())
		{
			partMessage += " :" + partMsg;
		}
		partMessage += "\r\n";
		broadcastToChannel(channelName, partMessage);

		Channel *channel = it->second;
		channel->removeMember(fd);

		if (channel->getMemberCount() == 0)
		{
			delete channel;
			channels.erase(it);
		}

		std::cout << "Client " << nickname << " left channel " << channelName << std::endl;
	}
}

void Multiplexer::sendPrivateMessage(int fd, const std::string &target, const std::string &message)
{
	std::string sender = getClientNickname(fd);
	if (sender.empty())
		return;

	if (target[0] == '#' || target[0] == '&')
	{

		if (isClientInChannel(fd, target))
		{
			std::string privmsg = ":" + sender + " PRIVMSG " + target + " :" + message + "\r\n";
			broadcastToChannel(target, privmsg, fd);
		}
		else
		{
			sendToClient(fd, "404 " + sender + " " + target + " :Cannot send to channel\r\n");
		}
	}
	else
	{

		int targetFd = findClientByNickname(target);
		if (targetFd != -1)
		{
			std::string privmsg = ":" + sender + " PRIVMSG " + target + " :" + message + "\r\n";
			sendToClient(targetFd, privmsg);
		}
		else
		{
			sendToClient(fd, "401 " + sender + " " + target + " :No such nick/channel\r\n");
		}
	}
}

void Multiplexer::sendNotice(int fd, const std::string &target, const std::string &message)
{
	std::string sender = getClientNickname(fd);
	if (sender.empty())
		return;

	if (target[0] == '#' || target[0] == '&')
	{

		if (isClientInChannel(fd, target))
		{
			std::string notice = ":" + sender + " NOTICE " + target + " :" + message + "\r\n";
			broadcastToChannel(target, notice, fd);
		}
	}
	else
	{

		int targetFd = findClientByNickname(target);
		if (targetFd != -1)
		{
			std::string notice = ":" + sender + " NOTICE " + target + " :" + message + "\r\n";
			sendToClient(targetFd, notice);
		}
	}
}

void Multiplexer::broadcastToClientChannels(int fd, const std::string &message)
{
	std::string nickname = getClientNickname(fd);
	if (nickname.empty())
		return;

	for (std::map<std::string, Channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		Channel *channel = it->second;
		if (channel->isMember(fd))
		{
			broadcastToChannel(it->first, message, fd);
		}
	}
}

void Multiplexer::broadcastToChannel(const std::string &channelName, const std::string &message)
{
	broadcastToChannel(channelName, message, -1);
}

void Multiplexer::broadcastToChannel(const std::string &channelName, const std::string &message, int excludeFd)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		Channel *channel = it->second;
		std::vector<int> clientList = channel->getAllMembers();
		for (std::vector<int>::iterator clientIt = clientList.begin(); clientIt != clientList.end(); ++clientIt)
		{
			if (*clientIt != excludeFd)
			{
				sendToClient(*clientIt, message);
			}
		}
	}
}

std::string Multiplexer::getChannelTopic(const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		return it->second->getTopic();
	}
	return "";
}

bool Multiplexer::setChannelTopic(int fd, const std::string &channelName, const std::string &topic)
{
	if (!isClientInChannel(fd, channelName))
	{
		sendToClient(fd, "442 " + getClientNickname(fd) + " " + channelName + " :You're not on that channel\r\n");
		return false;
	}

	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		it->second->setTopic(topic, getClientNickname(fd));
	}
	return true;
}

std::string Multiplexer::getChannelModes(const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		return it->second->getModes();
	}
	return "+";
}

void Multiplexer::setChannelMode(int fd, const std::string &channelName, const std::string &modeString, const std::vector<std::string> &params)
{

	if (isClientInChannel(fd, channelName))
	{
		std::map<std::string, Channel *>::iterator it = channels.find(channelName);
		if (it != channels.end())
		{

			for (size_t i = 0; i < modeString.length(); ++i)
			{
				char mode = modeString[i];
				if (mode == '+' || mode == '-')
					continue;
				bool add = (i == 0 || modeString[i - 1] == '+');
				it->second->setMode(mode, add);
			}
		}

		std::string nickname = getClientNickname(fd);
		std::string modeMsg = ":" + nickname + " MODE " + channelName + " " + modeString;

		for (size_t i = 0; i < params.size(); ++i)
		{
			modeMsg += " " + params[i];
		}
		modeMsg += "\r\n";

		broadcastToChannel(channelName, modeMsg);
	}
}

void Multiplexer::kickFromChannel(int fd, const std::string &channelName, const std::string &target, const std::string &reason)
{
	if (!isChannelOperator(fd, channelName))
	{
		sendToClient(fd, "482 " + getClientNickname(fd) + " " + channelName + " :You're not channel operator\r\n");
		return;
	}

	int targetFd = findClientByNickname(target);
	if (targetFd == -1 || !isClientInChannel(targetFd, channelName))
	{
		sendToClient(fd, "441 " + getClientNickname(fd) + " " + target + " " + channelName + " :They aren't on that channel\r\n");
		return;
	}

	std::string kicker = getClientNickname(fd);
	std::string kickMsg = ":" + kicker + " KICK " + channelName + " " + target + " :" + reason + "\r\n";
	broadcastToChannel(channelName, kickMsg);

	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		Channel *channel = it->second;
		channel->removeMember(targetFd);
	}
}

void Multiplexer::inviteToChannel(int fd, const std::string &target, const std::string &channelName)
{
	int targetFd = findClientByNickname(target);
	if (targetFd == -1)
	{
		sendToClient(fd, "401 " + getClientNickname(fd) + " " + target + " :No such nick/channel\r\n");
		return;
	}

	if (isClientInChannel(targetFd, channelName))
	{
		sendToClient(fd, "443 " + getClientNickname(fd) + " " + target + " " + channelName + " :is already on channel\r\n");
		return;
	}

	std::string inviter = getClientNickname(fd);
	std::string inviteMsg = ":" + inviter + " INVITE " + target + " " + channelName + "\r\n";
	sendToClient(targetFd, inviteMsg);

	sendToClient(fd, "341 " + inviter + " " + target + " " + channelName + "\r\n");
}

bool Multiplexer::isChannelOperator(int fd, const std::string &channelName)
{

	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end() && it->second->getMemberCount() > 0)
	{
		return it->second->isOperator(fd);
	}
	return false;
}

void Multiplexer::sendChannelNames(int fd, const std::string &channelName)
{
	std::string nickname = getClientNickname(fd);
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);

	if (it != channels.end())
	{
		std::string namesList = "";
		Channel *channel = it->second;
		std::vector<int> clientList = channel->getAllMembers();

		for (std::vector<int>::iterator clientIt = clientList.begin(); clientIt != clientList.end(); ++clientIt)
		{
			std::string clientNick = getClientNickname(*clientIt);
			if (!clientNick.empty())
			{
				if (!namesList.empty())
					namesList += " ";
				if (isChannelOperator(*clientIt, channelName))
				{
					namesList += "@";
				}
				namesList += clientNick;
			}
		}

		sendToClient(fd, "353 " + nickname + " = " + channelName + " :" + namesList + "\r\n");
	}

	sendToClient(fd, "366 " + nickname + " " + channelName + " :End of NAMES list\r\n");
}

void Multiplexer::sendAllChannelNames(int fd)
{
	for (std::map<std::string, Channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
	{
		sendChannelNames(fd, it->first);
	}
}

void Multiplexer::sendChannelList(int fd, const std::string &channels)
{
	std::string nickname = getClientNickname(fd);

	sendToClient(fd, "321 " + nickname + " Channel :Users  Name\r\n");

	if (channels.empty())
	{

		for (std::map<std::string, Channel *>::iterator it = this->channels.begin(); it != this->channels.end(); ++it)
		{
			std::ostringstream oss;
			oss << "322 " << nickname << " " << it->first << " " << it->second->getMemberCount();
			std::string topic = getChannelTopic(it->first);
			if (!topic.empty())
			{
				oss << " :" << topic;
			}
			oss << "\r\n";
			sendToClient(fd, oss.str());
		}
	}

	sendToClient(fd, "323 " + nickname + " :End of LIST\r\n");
}

std::string Multiplexer::getServerCreationTime()
{
	char timeStr[100];
	struct tm *timeinfo = localtime(&serverCreationTime);
	strftime(timeStr, sizeof(timeStr), "%a %b %d %Y at %H:%M:%S %Z", timeinfo);
	return std::string(timeStr);
}

int Multiplexer::findClientByNickname(const std::string &nickname)
{
	std::map<std::string, int>::iterator it = nicknameToFd.find(nickname);
	if (it != nicknameToFd.end())
	{
		return it->second;
	}
	return -1;
}

bool Multiplexer::isClientInChannel(int fd, const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		Channel *channel = it->second;
		return channel->isMember(fd);
	}
	return false;
}

void Multiplexer::handleWrite(int fd)
{

	(void)fd;
	std::cout << "Write event for client " << fd << " (not implemented yet)" << std::endl;
}

std::string Multiplexer::getClientFullIdentity(int fd)
{
	std::map<int, Client>::iterator it = clients.find(fd);
	if (it != clients.end())
	{
		std::string nickname = it->second.getNickname();
		std::string username = it->second.getUsername();

		return nickname + "!" + username;
	}
	return "";
}