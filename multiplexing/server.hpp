#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/epoll.h>
#include <exception>

class Server {
    private :
        int fd;
        sockaddr_in serverAddr;
    public :
        Server();
        Server(const Server &obj);
        ~Server();
        Server &operator=(const Server &obj);
        const int &getServerFd() const;
        sockaddr_in &getServerAddress();
        void setServerAddressSocket(sockaddr_in serverAddr);
};

#endif