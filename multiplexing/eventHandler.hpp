#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/epoll.h>
#include <exception>

class eventHandler{
    public : 
        virtual ~eventHandler() = 0;
        virtual void handleRead(int fd) = 0;
        virtual void handleWrite(int fd) = 0;
        virtual void handleError(int fd) = 0;
};

#endif
