#include "multiplexing.hpp"
#include <fcntl.h>
#include <arpa/inet.h>

Multiplexer::Multiplexer() : serverFd()
{
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
};

Multiplexer::Multiplexer(Server &serverSocket) : epoll_fd(epoll_create1(0))
{ // setting the epoll instance
	if (epoll_fd == -1)
		throw std::runtime_error("Error : error while creating epoll instance");
	setEpollInstance();
}

void Multiplexer::setEpollInstance()
{
	event.events = EPOLLIN | EPOLLOUT;		// Interested in readability and writability
	event.data.fd = serverFd.getServerFd(); // monitoring the socketfd
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
	int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead <= 0)
	{
		if (bytesRead == 0)
			std::cout << "Client disconnected\n";
		else
			std::cerr << "Can't connect to the client\n";
		if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL))
			throw std::runtime_error("Error while removing client from epoll");
		close(fd);
	}
	else
	{
		std::cout << "Client [" << fd << "]: " << buffer << std::endl;
		// Here Parse client message and stuff handle IRC commands
		Client *client = findFd(fd);
		client->appendToBuffer(buffer);
		// client->handleMessage(buffer);
	}
}
void Multiplexer::handleIRCCommand(Client *Client, std::string msg)
{
}
void Multiplexer::handleClientMessage(Client *client)
{
	std::string msg = client->getBuffer();
	if (msg.empty())
	{
		return;
	}

	// Remove trailing \r\n
	// std::string msg = rawMessage;
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
void Multiplexer::handleError(int fd)
{
	std::cerr << "Client error or hangup (fd: " << fd << ")" << std::endl;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL))
		throw std::runtime_error("Error while removing client from epoll");
	close(fd);
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

void Multiplexer::event_loop()
{
	while (1)
	{
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Error : Epoll wait failed");
		}
		for (int i = 0; i < nfds; i++)
		{
			if (events[i].data.fd == serverFd.getServerFd())
			{ // handle new clients
				sockaddr_in clientAddr;
				socklen_t sockAddressLen = sizeof(serverFd.getServerAddress());
				int clientFd = accept(serverFd.getServerFd(), (sockaddr *)&clientAddr, &sockAddressLen);
				if (clientFd == -1)
				{
					if (errno == EAGAIN || errno == EWOULDBLOCK)
						break;
					std::cerr << "Error accepting client\n";

					std::cout << "New client want to connect fd : " << clientFd << std::endl;
					;
					// print client info
					char clientIP[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
					std::cout << "Client IP: " << clientIP << ", Port: " << ntohs(clientAddr.sin_port) << std::endl;
					std::cout << "Client connected successfully.\n";
					// create a client object
					Client *client = new Client(clientFd, clientAddr);
					clients.insert(client);
				}
				std::cout << "Client connected with fd: " << clientFd << std::endl;

				fcntl(clientFd, F_SETFL, O_NONBLOCK);
				struct epoll_event clientEvent;
				clientEvent.events = EPOLLIN; // Only monitor for reads initially
				clientEvent.data.fd = clientFd;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1)
				{
					throw std::runtime_error("Error while adding client to epoll");
				} // can show the IP address of the client and port later
			}
			// should create event handler
			else
			{
				int clientFd = events[i].data.fd;

				// Handle read event
				if (events[i].events & EPOLLIN)
				{
					handleRead(clientFd);
				}

				// Handle write event if needed
				// if (events[i].events & EPOLLOUT) {
				//     // Implement when you need to send data to clients
				// }

				// Handle error conditions
				if (events[i].events & (EPOLLERR | EPOLLHUP))
				{
					std::cout << "Client error or hangup (fd: " << clientFd << ")" << std::endl;
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientFd, NULL);
					close(clientFd);
				}
			}
		}
	}
}