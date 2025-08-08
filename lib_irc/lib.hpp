#ifndef LIB_HPP
#define LIB_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/epoll.h>
#include <exception>

class Server;
class Multiplexer;

#include "../multiplexing/server.hpp"
#include "../multiplexing/multiplexing.hpp"

#endif