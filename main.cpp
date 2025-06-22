#include "lib_irc/lib.hpp"

//create a socket
//fill the socket address
//bind the socket with the address

//create an Epoll instance
//listen
//accept create a socket for the client and accepts requests
int main(){
    try{
        Server irc;
        Multiplexer obj(irc);
        // int nfds = epoll_wait(obj.getEpollFd(), obj.getEpollEvents(), MAX_EVENTS, -1);
        // std::cout << nfds <<  " eeeeee\n";
        // for(int i = 0; i < nfds; i++){
        //     std::cout << "event = " << obj.getEpollEvents()[i].data.fd << std::endl;
        //     std::cout << "event = " << obj.getEpollEvents()[i].events << std::endl;
        // }
    }
    catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
    }
}

