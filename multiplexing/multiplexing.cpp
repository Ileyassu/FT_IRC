#include "multiplexing.hpp"
#include "../IRC_msg_protocol/Message.hpp"
#include <fcntl.h>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>

Multiplexer *Multiplexer::instance = NULL;

Multiplexer::Multiplexer() : serverFd()
{
	instance = this;
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
};

Multiplexer::Multiplexer(int port, const std::string &password) : serverFd(port), serverPassword(password)
{
	instance = this;
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
};

Multiplexer::Multiplexer(Server &) : epoll_fd(epoll_create1(0))
{
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

void Multiplexer::handleRead(int fd)
{
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));

	int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			std::cout << "Client disconnected (fd: " << fd << ")\n";
		else
			std::cerr << "Error reading from client (fd: " << fd << ")\n";

		removeClient(fd);
		return;
	}

	buffer[bytesRead] = '\0';
	std::cout << "Client [" << fd << "]: " << buffer << std::endl;

	Client *client = findFd(fd);
	if (!client)
	{
		std::cerr << "Client not found for fd: " << fd << std::endl;
		removeClient(fd);
		return;
	}

	client->appendToBuffer(buffer);
	processClientMessages(client);
}

void Multiplexer::processClientMessages(Client *client)
{
	std::string buffer = client->getBuffer();
	size_t pos = 0;

	while ((pos = buffer.find("\r\n")) != std::string::npos || (pos = buffer.find("\n")) != std::string::npos)
	{
		std::string message = buffer.substr(0, pos);

		if (pos + 1 < buffer.length() && buffer.substr(pos, 2) == "\r\n")
			buffer.erase(0, pos + 2);
		else
			buffer.erase(0, pos + 1);

		if (!message.empty())
		{
			std::cout << "Processing IRC command: '" << message << "'" << std::endl;
			handleIRCCommand(client, message + "\r\n");
		}
	}

	client->setBuffer(buffer);
}

void Multiplexer::handleWrite(int fd)
{
	Client *client = findFd(fd);
	if (!client)
	{
		return;
	}

	std::cout << "Handle write for client fd: " << fd << std::endl;
}

void Multiplexer::removeClient(int fd)
{

	Client *client = findFd(fd);
	if (client)
	{

		clients.erase(client);

		delete client;
	}

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}

void Multiplexer::handleError(int fd)
{
	std::cerr << "Client error or hangup (fd: " << fd << ")" << std::endl;
	removeClient(fd);
}
void Multiplexer::handleNewConnection()
{
	sockaddr_in clientAddr;
	socklen_t sockAddressLen = sizeof(clientAddr);
	int clientFd = accept(serverFd.getServerFd(), (sockaddr *)&clientAddr, &sockAddressLen);

	if (clientFd == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;

		std::cerr << "Error accepting client connection\n";
		return;
	}

	std::cout << "New client connected with fd: " << clientFd << std::endl;

	char clientIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
	std::cout << "Client IP: " << clientIP << ", Port: " << ntohs(clientAddr.sin_port) << std::endl;

	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "Error setting client socket to non-blocking\n";
		close(clientFd);
		return;
	}

	Client *client = new Client(clientFd, clientAddr);
	clients.insert(client);

	struct epoll_event clientEvent;
	clientEvent.events = EPOLLIN | EPOLLET;
	clientEvent.data.fd = clientFd;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1)
	{
		std::cerr << "Error adding client to epoll\n";
		clients.erase(client);
		delete client;
		close(clientFd);
		return;
	}

	std::cout << "Client connected successfully.\n";
}

void Multiplexer::event_loop()
{
	while (1)
	{
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Error: Epoll wait failed");
		}

		for (int i = 0; i < nfds; i++)
		{
			if (events[i].data.fd == serverFd.getServerFd())
			{
				handleNewConnection();
			}
			else
			{
				int clientFd = events[i].data.fd;

				if (events[i].events & EPOLLIN)
				{
					handleRead(clientFd);
				}

				if (events[i].events & EPOLLOUT)
				{
					handleWrite(clientFd);
				}

				if (events[i].events & (EPOLLERR | EPOLLHUP))
				{
					handleError(clientFd);
				}
			}
		}
	}
}
void Multiplexer::handleIRCCommand(Client *client, std::string msg)
{
	std::cout << "handleIRCCommand called with: '" << msg << "'" << std::endl;
	try
	{

		Message *message = Message::createMessage(client->getSocketFd(), msg);

		if (message)
		{
			std::cout << "Message object created successfully" << std::endl;

			if (message->parse())
			{
				std::cout << "Message parsed successfully, executing..." << std::endl;

				message->excute(client);
			}
			else
			{

				std::cerr << "Invalid message format from client: " << client->getSocketFd() << std::endl;
			}

			delete message;
		}
		else
		{

			std::string command = extractCommand(msg);
			std::cerr << "Unknown command: " << command << " from client: " << client->getSocketFd() << std::endl;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Error processing message from client " << client->getSocketFd()
				  << ": " << e.what() << std::endl;
	}
}
void Multiplexer::handleClientMessage(Client *client)
{
	std::string msg = client->getBuffer();
	if (msg.empty())
	{
		return;
	}

	if (msg.length() >= 2 && msg.substr(msg.length() - 2) == "\r\n")
	{
		msg = msg.substr(0, msg.length() - 2);
	}
	else if (msg.length() >= 1 && (msg[msg.length() - 1] == '\r' || msg[msg.length() - 1] == '\n'))
	{
		msg = msg.substr(0, msg.length() - 1);
	}
	if (!msg.empty())
	{
		handleIRCCommand(client, msg);
	}
}

Client *Multiplexer::findFd(int fd)
{
	for (std::set<Client *>::iterator client = clients.begin(); client != clients.end(); ++client)
	{
		if ((*client)->getSocketFd() == fd)
			return *client;
	}
	return NULL;
}

Client *Multiplexer::findClientByNickname(const std::string &nickname)
{
	for (std::set<Client *>::iterator client = clients.begin(); client != clients.end(); ++client)
	{
		if ((*client)->getNickname() == nickname)
			return *client;
	}
	return NULL;
}

std::string Multiplexer::extractCommand(std::string rawMessage)
{
	std::istringstream iss(rawMessage);
	std::string command;

	if (rawMessage.empty() || rawMessage[0] == ':')
	{
		std::string prefix;
		iss >> prefix;
	}
	iss >> command;
	return command;
}

Channel *Multiplexer::getOrCreateChannel(const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
		return it->second;

	Channel *newChannel = new Channel(channelName);
	channels[channelName] = newChannel;
	return newChannel;
}

Channel *Multiplexer::getChannel(const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
		return it->second;
	return NULL;
}

void Multiplexer::removeChannel(const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		delete it->second;
		channels.erase(it);
	}
}

void Multiplexer::sendToClient(Client *client, const std::string &message)
{
	if (client)
	{
		send(client->getSocketFd(), message.c_str(), message.length(), 0);
	}
}

void Multiplexer::sendWelcomeMessages(Client *client)
{
	if (!client || client->getNickname().empty())
		return;

	std::string nick = client->getNickname();

	std::string welcome = ":server 001 " + nick + " :Welcome to the IRC Network " + nick + "!\r\n";
	sendToClient(client, welcome);

	std::string yourhost = ":server 002 " + nick + " :Your host is server, running version 1.0\r\n";
	sendToClient(client, yourhost);

	std::string created = ":server 003 " + nick + " :This server was created today\r\n";
	sendToClient(client, created);

	std::string myinfo = ":server 004 " + nick + " server 1.0 o o\r\n";
	sendToClient(client, myinfo);
}

Multiplexer *Multiplexer::getInstance()
{
	return instance;
}

bool Multiplexer::checkPassword(const std::string &password)
{
	return serverPassword == password;
}

bool Multiplexer::isClientInChannel(int fd, const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		Client *client = findFd(fd);
		if (client)
		{
			return it->second->isMember(client);
		}
	}
	return false;
}

bool Multiplexer::isChannelOperator(int fd, const std::string &channelName)
{
	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it != channels.end())
	{
		Client *client = findFd(fd);
		if (client)
		{
			return it->second->isOperator(client);
		}
	}
	return false;
}

void Multiplexer::kickFromChannel(int fd, const std::string &channelName, const std::string &target, const std::string &reason)
{
	Client *kicker = findFd(fd);
	if (!kicker)
		return;

	if (!isClientInChannel(fd, channelName) || !isChannelOperator(fd, channelName))
	{
		std::string error = ":server 482 " + kicker->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	Client *targetClient = findClientByNickname(target);
	if (!targetClient)
	{
		std::string error = ":server 401 " + kicker->getNickname() + " " + target + " :No such nick/channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (!isClientInChannel(targetClient->getSocketFd(), channelName))
	{
		std::string error = ":server 441 " + kicker->getNickname() + " " + target + " " + channelName + " :They aren't on that channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	Channel *channel = getChannel(channelName);
	if (!channel)
		return;

	std::string kickMsg = ":" + kicker->getNickname() + " KICK " + channelName + " " + target + " :" + reason + "\r\n";

	channel->broadcast(kickMsg, NULL);

	channel->removeMember(targetClient);

	std::cout << "KICK executed: " << kicker->getNickname() << " kicked " << target << " from " << channelName << " (" << reason << ")" << std::endl;
}

void Multiplexer::inviteToChannel(int fd, const std::string &target, const std::string &channelName)
{
	Client *inviter = findFd(fd);
	if (!inviter)
		return;

	Channel *channel = getChannel(channelName);
	if (!channel)
	{
		std::string error = ":server 403 " + inviter->getNickname() + " " + channelName + " :No such channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (!isClientInChannel(fd, channelName))
	{
		std::string error = ":server 442 " + inviter->getNickname() + " " + channelName + " :You're not on that channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (channel->isInviteOnly() && !isChannelOperator(fd, channelName))
	{
		std::string error = ":server 482 " + inviter->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	Client *targetClient = findClientByNickname(target);
	if (!targetClient)
	{
		std::string error = ":server 401 " + inviter->getNickname() + " " + target + " :No such nick/channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (isClientInChannel(targetClient->getSocketFd(), channelName))
	{
		std::string error = ":server 443 " + inviter->getNickname() + " " + target + " " + channelName + " :is already on channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	std::string response = ":server 341 " + inviter->getNickname() + " " + target + " " + channelName + "\r\n";
	send(fd, response.c_str(), response.length(), 0);

	std::string inviteMsg = ":" + inviter->getNickname() + " INVITE " + target + " " + channelName + "\r\n";
	send(targetClient->getSocketFd(), inviteMsg.c_str(), inviteMsg.length(), 0);

	std::cout << "INVITE executed: " << inviter->getNickname() << " invited " << target << " to " << channelName << std::endl;
}

bool Multiplexer::setChannelTopic(int fd, const std::string &channelName, const std::string &topic)
{
	Client *client = findFd(fd);
	if (!client)
		return false;

	std::map<std::string, Channel *>::iterator it = channels.find(channelName);
	if (it == channels.end())
	{
		std::string error = ":server 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return false;
	}

	Channel *channel = it->second;

	if (!isClientInChannel(fd, channelName))
	{
		std::string error = ":server 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return false;
	}

	if (channel->isTopicRestricted() && !isChannelOperator(fd, channelName))
	{
		std::string error = ":server 482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return false;
	}

	channel->setTopic(topic);

	std::string topicMsg = ":" + client->getNickname() + " TOPIC " + channelName + " :" + topic + "\r\n";
	channel->broadcast(topicMsg, NULL);

	std::cout << "TOPIC set by " << client->getNickname() << " in " << channelName << ": " << topic << std::endl;
	return true;
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

std::string Multiplexer::getChannelModes(const std::string &channelName)
{
	Channel *channel = getChannel(channelName);
	if (!channel)
		return "";

	std::string modes = "+";

	if (channel->isInviteOnly())
		modes += "i";
	if (channel->isTopicRestricted())
		modes += "t";
	if (!channel->getKey().empty())
		modes += "k";
	if (channel->getUserLimit() > 0)
		modes += "l";

	if (modes == "+")
		modes = "+";

	return modes;
}

void Multiplexer::setChannelMode(int fd, const std::string &channelName, const std::string &modeString, const std::vector<std::string> &params)
{
	Client *client = findFd(fd);
	if (!client)
		return;

	Channel *channel = getChannel(channelName);
	if (!channel)
	{
		std::string error = ":server 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (!isClientInChannel(fd, channelName))
	{
		std::string error = ":server 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	if (!isChannelOperator(fd, channelName))
	{
		std::string error = ":server 482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
		send(fd, error.c_str(), error.length(), 0);
		return;
	}

	bool adding = true;
	size_t paramIndex = 0;
	std::string appliedModes = "";
	std::vector<std::string> appliedParams;

	for (size_t i = 0; i < modeString.length(); ++i)
	{
		char mode = modeString[i];

		if (mode == '+')
		{
			adding = true;
			appliedModes += "+";
		}
		else if (mode == '-')
		{
			adding = false;
			appliedModes += "-";
		}
		else
		{

			switch (mode)
			{
			case 'i':
				channel->setInviteOnly(adding);
				appliedModes += "i";
				break;

			case 't':
				channel->setTopicRestricted(adding);
				appliedModes += "t";
				break;

			case 'k':
				if (adding && paramIndex < params.size())
				{
					channel->setKey(params[paramIndex]);
					appliedModes += "k";
					appliedParams.push_back(params[paramIndex]);
					paramIndex++;
				}
				else if (!adding)
				{
					channel->setKey("");
					appliedModes += "k";
				}
				break;

			case 'l':
				if (adding && paramIndex < params.size())
				{
					int limit = atoi(params[paramIndex].c_str());
					if (limit > 0)
					{
						channel->setUserLimit(limit);
						appliedModes += "l";
						appliedParams.push_back(params[paramIndex]);
					}
					paramIndex++;
				}
				else if (!adding)
				{
					channel->setUserLimit(0);
					appliedModes += "l";
				}
				break;

			case 'o':
				if (paramIndex < params.size())
				{
					Client *targetClient = findClientByNickname(params[paramIndex]);
					if (targetClient && isClientInChannel(targetClient->getSocketFd(), channelName))
					{
						if (adding)
							channel->addOperator(targetClient);
						else
							channel->removeOperator(targetClient);
						appliedModes += "o";
						appliedParams.push_back(params[paramIndex]);
					}
					paramIndex++;
				}
				break;

			default:

				std::string error = ":server 472 " + client->getNickname() + " " + mode + " :is unknown mode char to me\r\n";
				send(fd, error.c_str(), error.length(), 0);
				break;
			}
		}
	}

	if (!appliedModes.empty())
	{
		std::string modeMsg = ":" + client->getNickname() + " MODE " + channelName + " " + appliedModes;
		for (size_t i = 0; i < appliedParams.size(); ++i)
		{
			modeMsg += " " + appliedParams[i];
		}
		modeMsg += "\r\n";

		channel->broadcast(modeMsg, NULL);
		std::cout << "MODE executed: " << modeMsg;
	}
}
