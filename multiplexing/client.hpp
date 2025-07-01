#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/epoll.h>
#include <exception>
#include <map>
class Client {
    private :
        int fd;
        std::string nickname;
        std::string username;
        std::string hostname;
        bool isAuthenticated;
        std::vector<std::string> channels;
    public :
        Client();
        ~Client();
        int getFd();
        std::string getNickname();
        std::string getUsername();
        std::string getHostname();
        bool getIsAuthenticated();
};

#endif