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

Server::Server(const Server &obj){
    *this = obj;
}

Server::~Server(){
    close(fd);
    std::cout << "Server is down.\n";
}

Server &Server::operator=(const Server &obj){
    if(this == &obj)
        return *this;
    this->fd = obj.fd;
    this->serverAddr.sin_addr = obj.serverAddr.sin_addr;
    this->serverAddr.sin_family = obj.serverAddr.sin_family;
    this->serverAddr.sin_port = obj.serverAddr.sin_port;
    return *this;
}

void Server::setServerAddressSocket(sockaddr_in serverAddr)
{
    this->serverAddr.sin_family = serverAddr.sin_family;
    this->serverAddr.sin_port = serverAddr.sin_port;
    this->serverAddr.sin_addr = serverAddr.sin_addr;
}

const int &Server::getServerFd() const {
    return fd;
}

sockaddr_in &Server::getServerAddress(){
    return serverAddr;
}