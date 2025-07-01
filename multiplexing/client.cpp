#include "client.hpp"

Client::Client(){}
Client::~Client(){
    close(fd);
}

int Client::getFd(){
    return fd;
}

std::string Client::getNickname()
{
    return nickname;
}

std::string Client::getUsername()
{
    return username;
}

std::string Client::getHostname()
{
    return hostname;
}

bool Client::getIsAuthenticated(){
    return isAuthenticated;
}