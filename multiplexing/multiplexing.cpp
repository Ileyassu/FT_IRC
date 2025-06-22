#include "multiplexing.hpp"

Multiplexer::Multiplexer(Server &serverSocket):epoll_fd(epoll_create1(0)){ //setting the epoll instance
    if(epoll_fd == -1)
        throw std::runtime_error("Error : error while creating epoll instance");
    event.events = EPOLLIN | EPOLLOUT;// Interested in readability and writability
    event.data.fd = serverSocket.getServerFd(); //monitoring the socketfd
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket.getServerFd(), &event) == -1)
        throw std::runtime_error("Error : error while monitoring fd (epoll_ctl)");
}

Multiplexer::~Multiplexer(){
    close(epoll_fd);
}

const int &Multiplexer::getEpollFd(){
    return epoll_fd;
}

epoll_event *Multiplexer::getEpollEvents(){
    return events;
}

void Multiplexing::event_loop(){
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
}