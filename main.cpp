#include "lib_irc/lib.hpp"

//create a socket
//fill the socket address
//bind the socket with the address

//create an Epoll instance
//listen
//accept create a socket for the client and accepts requests
int main(){
    try {
            Multiplexer obj;
            std::cout << "Server is listening in FD = " << obj.getServer().getServerFd() << std::endl;
            obj.event_loop();
        }   
    catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
    }
}