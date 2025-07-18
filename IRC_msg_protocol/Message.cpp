#include "Message.hpp"



Message::Message(/* args */)
{
}

Message::~Message()
{
}

MessageFactory::MessageFactory(/* args */)
{
}


JoinCommand::JoinCommand(int fd, std::string  rawMessage) : Message(fd ,rawMessage)
{

}


