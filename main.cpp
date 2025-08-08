#include "lib_irc/lib.hpp"
#include "IRC_msg_protocol/Message.hpp"
#include <cstdlib>

#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <exception>

// Global flag for graceful shutdown
bool server_shutdown = false;

void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        std::cout << "\nReceived SIGINT. Shutting down gracefully..." << std::endl;
        server_shutdown = true;
    }
}

bool isValidPort(const std::string& portStr, int& port)
{
    char* endptr;
    long val = std::strtol(portStr.c_str(), &endptr, 10);
    
    // Check if conversion was successful and entire string was consumed
    if (*endptr != '\0' || endptr == portStr.c_str())
        return false;
    
    // Check range
    if (val <= 0 || val > 65535)
        return false;
    
    port = static_cast<int>(val);
    return true;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    
    int port;
    std::string password = argv[2];

    if (!isValidPort(argv[1], port))
    {
        std::cerr << "Error: Invalid port number. Must be between 1 and 65535." << std::endl;
        return 1;
    }
    
    if (password.empty())
    {
        std::cerr << "Error: Password cannot be empty." << std::endl;
        return 1;
    }
    
    try
    {
        Multiplexer obj(port, password);
        std::cout << "Server is listening on port " << port << " with FD = " 
                  << obj.getServer().getServerFd() << std::endl;
        std::cout << "Press Ctrl+C to shutdown gracefully." << std::endl;
        
        obj.event_loop();
        obj.cleanUpServer();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Server shutdown complete." << std::endl;
    return 0;
}