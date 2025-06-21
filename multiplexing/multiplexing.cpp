#include "multiplexing.hpp"

Multiplexer::Multiplexer(Server &serverSocket):epoll_fd(epoll_create1(0)){
    if(epoll_fd == -1)
        throw std::runtime_error("Error : error while creating epoll instance");
    
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket.getServerFd(), &event) == -1)
        throw std::runtime_error("Error : error while monitoring fd (epoll_ctl)");

}