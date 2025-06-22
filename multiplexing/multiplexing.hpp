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

class Multiplexer {
    private :
        Server serverSocket;
        const int epoll_fd;
        struct epoll_event event, events[10];

    public : 
        Multiplexer(Server &serverSocket);
        ~Multiplexer();
};

#endif