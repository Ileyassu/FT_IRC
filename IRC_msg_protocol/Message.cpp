#include "Message.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "../multiplexing/multiplexing.hpp"
#include <cctype>
#include <sys/socket.h>

// Helper function to clean IRC messages
std::string cleanMessage(const std::string& msg) {
    std::string cleanMsg = msg;
    cleanMsg.erase(cleanMsg.find_last_not_of("\r\n") + 1);
    cleanMsg += "\r\n";
    return cleanMsg;
}

Message::Message() : clientFD(-1)
{
}

Message::Message(int fd, std::string rawMessage) : clientFD(fd), msg(rawMessage)
{
}

Message::~Message()
{
}

bool Message::parse()
{
	std::cout << "call here" << std::endl;
	if (msg.empty())
		return false;

	std::string line = msg;
	size_t pos = 0;

	if (line.length() >= 2 && line.substr(line.length() - 2) == "\r\n")
		line = line.substr(0, line.length() - 2);
	else if (line.length() >= 1 && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n'))
		line = line.substr(0, line.length() - 1);

	if (!line.empty() && line[0] == ':')
	{
		size_t spacePos = line.find(' ');
		if (spacePos == std::string::npos)
			return false;
		prefix = line.substr(1, spacePos - 1);
		pos = spacePos + 1;
	}

	size_t cmdEnd = line.find(' ', pos);
	if (cmdEnd == std::string::npos)
	{
		command = line.substr(pos);
		return true;
	}

	command = line.substr(pos, cmdEnd - pos);
	pos = cmdEnd + 1;

	while (pos < line.length())
	{
		if (line[pos] == ':')
		{

			params.push_back(line.substr(pos + 1));
			break;
		}

		size_t paramEnd = line.find(' ', pos);
		if (paramEnd == std::string::npos)
		{
			params.push_back(line.substr(pos));
			break;
		}

		params.push_back(line.substr(pos, paramEnd - pos));
		pos = paramEnd + 1;
	}

	return true;
}

int Message::getClientFd() const
{
	return clientFD;
}

const std::string &Message::getCommand() const
{
	return command;
}

const std::vector<std::string> &Message::getParams() const
{
	return params;
}

const std::string &Message::getPrefix() const
{
	return prefix;
}

const std::string &Message::getRawMessage() const
{
	return msg;
}

bool Message::hasPrefix() const
{
	return !prefix.empty();
}

size_t Message::getParamCount() const
{
	return params.size();
}

std::string Message::getParam(size_t index) const
{
	if (index < params.size())
		return params[index];
	return "";
}

bool Message::isCommand(const std::string &cmd) const
{
	std::string upperCmd = cmd;
	std::string upperCommand = command;

	for (size_t i = 0; i < upperCmd.length(); ++i)
		upperCmd[i] = std::toupper(upperCmd[i]);
	for (size_t i = 0; i < upperCommand.length(); ++i)
		upperCommand[i] = std::toupper(upperCommand[i]);

	return upperCommand == upperCmd;
}

std::string Message::getTrailingParam() const
{
	if (params.empty())
		return "";
	return params[params.size() - 1];
}

bool Message::hasTrailingParam() const
{
	return !params.empty() && msg.find(" :") != std::string::npos;
}
Message *Message::createMessage(int fd, const std::string &raw_msg)
{
    Message* message = NULL;
    // Parse command directly without creating temporary object
    std::string line = raw_msg;
    
    // Remove CRLF
    if (line.length() >= 2 && line.substr(line.length() - 2) == "\r\n")
        line = line.substr(0, line.length() - 2);
    else if (line.length() >= 1 && (line[line.length() - 1] == '\r' || line[line.length() - 1] == '\n'))
        line = line.substr(0, line.length() - 1);
    
    // Skip prefix if present
    size_t pos = 0;
    if (!line.empty() && line[0] == ':')
    {
        size_t spacePos = line.find(' ');
        if (spacePos == std::string::npos)
            return NULL;
        pos = spacePos + 1;
    }
    
    // Extract command
    size_t cmdEnd = line.find(' ', pos);
    std::string cmd;
    if (cmdEnd == std::string::npos)
        cmd = line.substr(pos);
    else
        cmd = line.substr(pos, cmdEnd - pos);
    
    // Convert to uppercase
    for (size_t i = 0; i < cmd.length(); ++i)
    {
        cmd[i] = std::toupper(cmd[i]);
    }
    
    std::cout << "Creating message for command: " << cmd << std::endl;
    
    // Create appropriate command object (parse() called only once in constructor)
    if (cmd == "PASS")
        message = new PassCommand(fd, raw_msg);
    else if (cmd == "JOIN")
        message = new JoinCommand(fd, raw_msg);
    else if (cmd == "PRIVMSG")
        message = new PrivmsgCommand(fd, raw_msg);
    else if (cmd == "NICK")
        message = new NickCommand(fd, raw_msg);
    else if (cmd == "USER")
        message = new UserCommand(fd, raw_msg);
    else if (cmd == "PART")
        message = new PartCommand(fd, raw_msg);
    else if (cmd == "QUIT")
        message = new QuitCommand(fd, raw_msg);
    else if (cmd == "PING")
        message = new PingCommand(fd, raw_msg);
    else if (cmd == "KICK")
        message = new KickCommand(fd, raw_msg);
    else if (cmd == "INVITE")
        message = new InviteCommand(fd, raw_msg);
    else if (cmd == "TOPIC")
        message = new TopicCommand(fd, raw_msg);
    else if (cmd == "MODE")
        message = new ModeCommand(fd, raw_msg);
    else
        message = new UnknownCommand(fd, raw_msg);
    

    if (message && !message->parse())
    {
        delete message;
        message = NULL;
    }
    
    return message;
}

MessageFactory::MessageFactory()
{
}

JoinCommand::JoinCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void JoinCommand::excute(Client *client)
{
	std::cout << "Executing JOIN command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string channelName = getParam(0);
		std::cout << " Channel: " << channelName;

		if (client != NULL)
		{

			if (!client->isClientRegistered())
			{
				std::string error = ":server 451 * :You have not registered\r\n";
				error = cleanMessage(error);
				send(clientFD, error.c_str(), error.length(), 0);
				return;
			}

			Multiplexer *server = Multiplexer::getInstance();

			if (server)
			{

				Channel *channel = server->getOrCreateChannel(channelName);

				if (channel)
				{

					if (channel->addMember(client))
					{
						std::string nickname = client->getNickname();
						std::string username = client->getUsername();

						std::string joinMsg = ":" + nickname + "!" + username + "@localhost JOIN " + channelName + "\r\n";
						joinMsg = cleanMessage(joinMsg);
						send(clientFD, joinMsg.c_str(), joinMsg.length(), 0);

						std::string topicMsg = ":server 332 " + nickname + " " + channelName + " :" + channel->getTopic() + "\r\n";
						topicMsg = cleanMessage(topicMsg);
						send(clientFD, topicMsg.c_str(), topicMsg.length(), 0);

						std::string names = ":server 353 " + nickname + " = " + channelName + " :";
						std::vector<Client *> members = channel->getMembers();
						for (size_t i = 0; i < members.size(); ++i)
						{
							if (channel->isOperator(members[i]))
								names += "@";
							names += members[i]->getNickname();
							if (i < members.size() - 1)
								names += " ";
						}
						names += "\r\n";
						names = cleanMessage(names);
						send(clientFD, names.c_str(), names.length(), 0);

						std::string endNames = ":server 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
						endNames = cleanMessage(endNames);
						send(clientFD, endNames.c_str(), endNames.length(), 0);

						std::string notifyMsg = ":" + nickname + "!" + username + "@localhost JOIN " + channelName + "\r\n";
						notifyMsg = cleanMessage(notifyMsg);
						channel->broadcastToOthers(notifyMsg, client);
					}
				}
			}
		}
	}
	std::cout << std::endl;
}
PrivmsgCommand::PrivmsgCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void PrivmsgCommand::excute(Client *client)
{
    if (client == NULL) {
        std::cerr << "Error: Client is NULL in PRIVMSG command" << std::endl;
        return;
    }

    int clientFD = getClientFd(); 
    
    std::cout << "Executing PRIVMSG command for client FD: " << clientFD;
    

if (getParamCount() < 2) {
    std::string nickname = client->getNickname();
    std::string error = ":server 461 " + nickname + " PRIVMSG :Not enough parameters\r\n";
    error = cleanMessage(error);
    send(clientFD, error.c_str(), error.length(), 0);
    return;
}

std::string target = getParam(0);
std::string message;

// Validate target parameter
if (target.empty()) {
    std::string nickname = client->getNickname();
    std::string error = ":server 461 " + nickname + " PRIVMSG :Not enough parameters\r\n";
    error = cleanMessage(error);
    send(clientFD, error.c_str(), error.length(), 0);
    return;
}

if (getParamCount() >= 2) {
    // Simply take parameter 1 as the start of the message
    message = getParam(1);
    
    // Add any additional parameters with spaces
    for (size_t i = 2; i < getParamCount(); i++) {
        message += " " + getParam(i);
    }
    
    // Remove leading ':' if present (IRC trailing parameter syntax)
    if (!message.empty() && message[0] == ':') {
        message = message.substr(1);
    }
}



// Validate message isn't empty
if (message.empty()) {
    std::string nickname = client->getNickname();
    std::string error = ":server 412 " + nickname + " :No text to send\r\n";
    error = cleanMessage(error);
    send(clientFD, error.c_str(), error.length(), 0);
    return;
}

    std::cout << " Target: " << target << " Message: " << message << std::endl;

    std::string nickname = client->getNickname();
    std::string username = client->getUsername();
    
    // Check if target is a channel (starts with #)
    if (!target.empty() && target[0] == '#') {
        Multiplexer *server = Multiplexer::getInstance();
        if (server) {
            Channel *channel = server->getChannel(target);
            if (channel) {
                if (channel->isMember(client)) {
					
                    std::string channelMsg = ":" + nickname + "!" + username + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
					channelMsg = cleanMessage(channelMsg);
                    channel->broadcastToOthers(channelMsg, client);
                } else {
                    // Client is not a member of the channel
                    std::string error = ":server 404 " + nickname + " " + target + " :Cannot send to channel\r\n";
                    error = cleanMessage(error);
                    send(clientFD, error.c_str(), error.length(), 0);
                }
            } else {
                // Channel doesn't exist
                std::string error = ":server 403 " + nickname + " " + target + " :No such channel\r\n";
                error = cleanMessage(error);
                send(clientFD, error.c_str(), error.length(), 0);
            }
        }
    } else {
        // Target is a user
        Multiplexer *server = Multiplexer::getInstance();
        if (server) {
            Client *targetClient = server->findClientByNickname(target);
            if (targetClient) {
                std::string userMsg = ":" + nickname + "!" + username + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
                userMsg = cleanMessage(userMsg);
                send(targetClient->getSocketFd(), userMsg.c_str(), userMsg.length(), 0);
            } else {
                // Target user doesn't exist
                std::string error = ":server 401 " + nickname + " " + target + " :No such nick/channel\r\n";
                error = cleanMessage(error);
                send(clientFD, error.c_str(), error.length(), 0);
            }
        }
    }
}
NickCommand::NickCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void NickCommand::excute(Client *client)
{
	std::cout << "Executing NICK command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string nickname = getParam(0);
		std::cout << " Nickname: " << nickname;

		if (client != NULL)
		{

			if (!client->isClientAuthenticated())
			{
				std::string response = ":server 464 * :Password required\r\n";
				response = cleanMessage(response);
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication required";
				std::cout << std::endl;
				return;
			}

			std::string oldNick = client->getNickname();

			client->setNickname(nickname);
			std::cout << " (Old Nick: " << oldNick << ")";
			if (oldNick == nickname)
			{
				std::cout << " - Nickname unchanged";
				std::cout << std::endl;
				return;
			}

			if (oldNick.empty())
			{
				std::cout << " (First time setting nickname)";
				std::cout << std::endl;
				
			}
			else
			{

				std::string response = ":" + oldNick + " NICK :" + nickname + "\r\n";
				response = cleanMessage(response);
				send(clientFD, response.c_str(), response.length(), 0);
			}

			if (!client->getUsername().empty() && !client->isClientRegistered())
			{
				client->setRegistered(true);

				std::string welcome1 = ":server 001 " + nickname + " :Welcome to the IRC Network, " + nickname + "\r\n";
				std::string welcome2 = ":server 002 " + nickname + " :Your host is server, running version 1.0\r\n";
				std::string welcome3 = ":server 003 " + nickname + " :This server was created today\r\n";
				std::string welcome4 = ":server 004 " + nickname + " server 1.0 o o\r\n";
				send(clientFD, welcome1.c_str(), welcome1.length(), 0);
				send(clientFD, welcome2.c_str(), welcome2.length(), 0);
				send(clientFD, welcome3.c_str(), welcome3.length(), 0);
				send(clientFD, welcome4.c_str(), welcome4.length(), 0);
			}
		}
	}
	std::cout << std::endl;
}
UserCommand::UserCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void UserCommand::excute(Client *client)
{
	std::cout << "Executing USER command for client FD: " << clientFD;
	if (getParamCount() >= 4)
	{
		std::string username = getParam(0);
		std::string realname = getParam(3);
		std::cout << " Username: " << username << " Realname: " << realname;

		if (client != NULL)
		{

			if (!client->isClientAuthenticated())
			{
				std::string response = ":server 464 * :Password required\r\n";
				response = cleanMessage(response);
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication required";
				std::cout << std::endl;
				return;
			}

			client->setUsername(username);
			client->setRealname(realname);
			std::string identMsg = ":server 001 * :You are now identified\r\n";
			identMsg = cleanMessage(identMsg);
			send(clientFD, identMsg.c_str(), identMsg.length(), 0);

			if (!client->getNickname().empty() && !client->isClientRegistered())
			{
				client->setRegistered(true);
				std::string nickname = client->getNickname();

				std::string welcome1 = ":server 001 " + nickname + " :Welcome to the IRC Network, " + nickname + "\r\n";
				std::string welcome2 = ":server 002 " + nickname + " :Your host is server, running version 1.0\r\n";
				std::string welcome3 = ":server 003 " + nickname + " :This server was created today\r\n";
				std::string welcome4 = ":server 004 " + nickname + " server 1.0 o o\r\n";
				send(clientFD, welcome1.c_str(), welcome1.length(), 0);
				send(clientFD, welcome2.c_str(), welcome2.length(), 0);
				send(clientFD, welcome3.c_str(), welcome3.length(), 0);
				send(clientFD, welcome4.c_str(), welcome4.length(), 0);
			}
		}
	}
	std::cout << std::endl;
}
PartCommand::PartCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void PartCommand::excute(Client *client)
{
	std::cout << "Executing PART command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string channelName = getParam(0);
		std::cout << " Channel: " << channelName;

		if (client != NULL)
		{

			if (!client->isClientRegistered())
			{
				std::string error = ":server 451 * :You have not registered\r\n";
				error = cleanMessage(error);
				send(clientFD, error.c_str(), error.length(), 0);
				return;
			}

			Multiplexer *server = Multiplexer::getInstance();
			if (server)
			{
				Channel *channel = server->getChannel(channelName);
				if (channel && channel->isMember(client))
				{
					std::string nickname = client->getNickname();
					std::string username = client->getUsername();
					std::string reason = (getParamCount() > 1) ? getParam(1) : "Leaving";

					std::string partMsg = ":" + nickname + "!" + username + "@localhost PART " + channelName + " :" + reason + "\r\n";
					partMsg = cleanMessage(partMsg);
					send(clientFD, partMsg.c_str(), partMsg.length(), 0);

					channel->broadcastToOthers(partMsg, client);

					channel->removeMember(client);

					if (channel->getMemberCount() == 0)
					{
						server->removeChannel(channelName);
					}
				}
				else if (!channel)
				{

					std::string error = ":server 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n";
					error = cleanMessage(error);
					send(clientFD, error.c_str(), error.length(), 0);
				}
				else
				{

					std::string error = ":server 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n";
					error = cleanMessage(error);
					send(clientFD, error.c_str(), error.length(), 0);
				}
			}
		}
	}
	std::cout << std::endl;
}

QuitCommand::QuitCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	
}

void QuitCommand::excute(Client *client)
{
	std::cout << "Executing QUIT command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string reason = getParam(0);
		std::cout << " Reason: " << reason;

		if (client != NULL)
		{
			std::string nickname = client->getNickname();

			std::string response = ":" + nickname + " QUIT :Quit: " + reason + "\r\n";
			response = cleanMessage(response);
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	else
	{
		if (client != NULL)
		{
			std::string nickname = client->getNickname();

			std::string response = ":" + nickname + " QUIT :Client Quit\r\n";
			response = cleanMessage(response);
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	std::cout << " - Client will be disconnected" << std::endl;
}

PingCommand::PingCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	
}

void PingCommand::excute(Client *client)
{
	std::cout << "Executing PING command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string server = getParam(0);
		std::cout << " Server: " << server;

		if (client != NULL)
		{

			std::string response = ":server PONG server :" + server + "\r\n";
			response = cleanMessage(response);
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	else
	{
		if (client != NULL)
		{

			std::string response = ":server PONG server\r\n";
			response = cleanMessage(response);
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	std::cout << std::endl;
}

UnknownCommand::UnknownCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	
}

void UnknownCommand::excute(Client *client)
{
	(void)client;
	std::cout << "Executing UNKNOWN command '" << command << "' for client FD: " << clientFD << std::endl;
}

PassCommand::PassCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	
}

void PassCommand::excute(Client *client)
{
	std::cout << "Executing PASS command for client FD: " << clientFD;
	if (getParamCount() > 0)
	{
		std::string password = getParam(0);
		std::cout << " Password attempt";

		if (client != NULL)
		{

			if (Multiplexer::getInstance() && Multiplexer::getInstance()->checkPassword(password))
			{
				client->setAuthenticated(true);
				std::cout << " - Authentication successful";
				std::string authMsg = ":server 001 * :You are now authenticated\r\n";
				authMsg = cleanMessage(authMsg);
				send(clientFD, authMsg.c_str(), authMsg.length(), 0);
			}
			else
			{

				std::string response = ":server 464 * :Password incorrect\r\n";
				response = cleanMessage(response);
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication failed";
			}
		}
	}
	std::cout << std::endl;
}


KickCommand::KickCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	
}

void KickCommand::excute(Client *client)
{
	std::cout << "Executing KICK command for client FD: " << clientFD;

	if (getParamCount() >= 2)
	{
		std::string channelName = getParam(0);
		std::string targetNick = getParam(1);
		std::string reason = (getParamCount() >= 3) ? getParam(2) : "Kicked by operator";

		std::cout << " Channel: " << channelName << " Target: " << targetNick << " Reason: " << reason;

		if (client != NULL && client->isClientAuthenticated() && !client->getNickname().empty())
		{

			Multiplexer *server = Multiplexer::getInstance();
			if (server)
			{
				if (!server->isClientInChannel(clientFD, channelName))
				{
					std::string response = ":server 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n";
					response = cleanMessage(response);
					send(clientFD, response.c_str(), response.length(), 0);
				}
				else if (!server->isChannelOperator(clientFD, channelName))
				{
					std::string response = ":server 482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
					response = cleanMessage(response);
					send(clientFD, response.c_str(), response.length(), 0);
				}
				else
				{
					server->kickFromChannel(clientFD, channelName, targetNick, reason);
				}
			}
		}
	}
	std::cout << std::endl;
}

InviteCommand::InviteCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void InviteCommand::excute(Client *client)
{
	std::cout << "Executing INVITE command for client FD: " << clientFD;

	if (getParamCount() >= 2)
	{
		std::string targetNick = getParam(0);
		std::string channelName = getParam(1);

		std::cout << " Target: " << targetNick << " Channel: " << channelName;

		if (client != NULL && client->isClientAuthenticated() && !client->getNickname().empty())
		{
			Multiplexer *server = Multiplexer::getInstance();
			if (server)
			{
				server->inviteToChannel(clientFD, targetNick, channelName);
			}
		}
	}
	std::cout << std::endl;
}

TopicCommand::TopicCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
}

void TopicCommand::excute(Client *client)
{
	std::cout << "Executing TOPIC command for client FD: " << clientFD;

	if (getParamCount() >= 1)
	{
		std::string channelName = getParam(0);
		std::cout << " Channel: " << channelName;

		if (client != NULL && client->isClientAuthenticated() && !client->getNickname().empty())
		{
			Multiplexer *server = Multiplexer::getInstance();
			if (server)
			{
				if (getParamCount() >= 2)
				{

					std::string newTopic = getParam(1);
					std::cout << " New Topic: " << newTopic;
					server->setChannelTopic(clientFD, channelName, newTopic);
				}
				else
				{

					std::string topic = server->getChannelTopic(channelName);
					if (!topic.empty())
					{
						std::string response = ":server 332 " + client->getNickname() + " " + channelName + " :" + topic + "\r\n";
						response = cleanMessage(response);
						send(clientFD, response.c_str(), response.length(), 0);
					}
					else
					{
						std::string response = ":server 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
						response = cleanMessage(response);
						send(clientFD, response.c_str(), response.length(), 0);
					}
				}
			}
		}
	}
	std::cout << std::endl;
}

ModeCommand::ModeCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{

}

void ModeCommand::excute(Client *client)
{
	std::cout << "Executing MODE command for client FD: " << clientFD;

	if (getParamCount() >= 1)
	{
		std::string target = getParam(0);
		std::cout << " Target: " << target;

		if (client != NULL && client->isClientAuthenticated() && !client->getNickname().empty())
		{
			if (target[0] == '#' || target[0] == '&')
			{

				Multiplexer *server = Multiplexer::getInstance();
				if (server)
				{
					if (getParamCount() >= 2)
					{

						std::string modeString = getParam(1);
						std::vector<std::string> params;
						for (size_t i = 2; i < getParamCount(); ++i)
						{
							params.push_back(getParam(i));
						}
						std::cout << " Mode: " << modeString;
						server->setChannelMode(clientFD, target, modeString, params);
					}
					else
					{

						std::string modes = server->getChannelModes(target);
						std::string response = ":server 324 " + client->getNickname() + " " + target + " " + modes + "\r\n";
						response = cleanMessage(response);
						send(clientFD, response.c_str(), response.length(), 0);
					}
				}
			}
			else
			{

				if (target == client->getNickname())
				{
					std::string response = ":server 221 " + client->getNickname() + " +\r\n";
					response = cleanMessage(response);
					send(clientFD, response.c_str(), response.length(), 0);
				}
			}
		}
	}
	std::cout << std::endl;
}