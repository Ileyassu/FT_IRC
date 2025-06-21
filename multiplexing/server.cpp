#include "server.hpp"

Server::Server():fd(socket(AF_INET, SOCK_STREAM, 0)){
    if(fd == -1)
        throw std::runtime_error("Error : Server FD");
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(fd, (const sockaddr *)&serverAddr,  sizeof(serverAddr)) == -1)
        throw std::runtime_error("Error : Binding error");
}

Server::~Server(){
    close(fd);
    std::cout << "Server is down.\n";
}

const int &Server::getServerFd() const {
    return fd;
}

sockaddr_in &Server::getServerAddress(){
    return serverAddr;
}