#ifndef SERVER_HPP
#define SERVER_HPP

#include "../lib_irc/lib.hpp"

class Server {
    private :
        const int fd;
        sockaddr_in serverAddr;
    public :
        Server();
        ~Server();
        const int &getServerFd() const;
        sockaddr_in &getServerAddress();
};

#endif