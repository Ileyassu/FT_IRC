#ifndef MULTIPLEXING_HPP
#define MULTIPLEXING_HPP

#include "../lib_irc/lib.hpp"

class Multiplexer {
    private :
        Server serverSocket;
        const int epoll_fd;
        struct epoll_event event, events[MAX_EVENTS];

    public : 
        Multiplexer(Server &serverSocket);
        ~Multiplexer();
};

#endif