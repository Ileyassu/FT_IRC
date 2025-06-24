#ifndef REACTOR_HPP
#define REACTOR_HPP

#include "../lib_irc/lib.hpp"

class Reactor {
    private :
        Multiplexer multiplex;
    public :
        Reactor();
        ~Reactor();
};

#endif