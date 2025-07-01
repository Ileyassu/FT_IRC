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

#define MAX_EVENTS 1024
class Multiplexer {
    private :
        int epoll_fd;
        Server serverFd;
        struct epoll_event event, events[MAX_EVENTS];

    public : 
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
};

#endif