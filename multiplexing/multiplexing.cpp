#include "multiplexing.hpp"
#include <fcntl.h>

Multiplexer::Multiplexer(): serverFd(){
    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
        throw std::runtime_error("Error : error while creating epoll instance");
    setEpollInstance();
};

Multiplexer::Multiplexer(Server &serverSocket):epoll_fd(epoll_create1(0)){ //setting the epoll instance
    if(epoll_fd == -1)
        throw std::runtime_error("Error : error while creating epoll instance");
    setEpollInstance();
}

void Multiplexer::setEpollInstance(){
    if(epoll_fd == -1)
        throw std::runtime_error("Error : error while creating epoll instance");
    event.events = EPOLLIN | EPOLLOUT;// Interested in readability and writability
    event.data.fd = serverFd.getServerFd(); //monitoring the socketfd
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverFd.getServerFd(), &event) == -1)
        throw std::runtime_error("Error : error while monitoring fd (epoll_ctl)");
    if(listen(serverFd.getServerFd(), 128) == -1)
        throw std::runtime_error("Error : passive socket error, while listening");
    if(fcntl(serverFd.getServerFd(), F_SETFL, O_NONBLOCK))
        throw std::runtime_error("Error : fcntl error");
    std::cout << "server FD = " << serverFd.getServerFd() << std::endl;
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

Server &Multiplexer::getServer(){
    return serverFd;
}



// void Multiplexer::event_loop(){    
//     while(42){
//         int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
//         for(int i = 0; i < nfds; i++){
//             if(){

//             }
//         }
//     }
// }