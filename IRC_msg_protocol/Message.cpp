#include "Message.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "../multiplexing/multiplexing.hpp"
#include <cctype>
#include <sys/socket.h>

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

	Message *tempMsg = new JoinCommand(fd, raw_msg);
	if (!tempMsg->parse())
	{
		delete tempMsg;
		return NULL;
	}

	std::string cmd = tempMsg->getCommand();
	delete tempMsg;

	for (size_t i = 0; i < cmd.length(); ++i)
	{
		cmd[i] = std::toupper(cmd[i]);
	}

	if (cmd == "PASS")
		return new PassCommand(fd, raw_msg);
	else if (cmd == "JOIN")
		return new JoinCommand(fd, raw_msg);
	else if (cmd == "PRIVMSG")
		return new PrivmsgCommand(fd, raw_msg);
	else if (cmd == "NICK")
		return new NickCommand(fd, raw_msg);
	else if (cmd == "USER")
		return new UserCommand(fd, raw_msg);
	else if (cmd == "PART")
		return new PartCommand(fd, raw_msg);
	else if (cmd == "QUIT")
		return new QuitCommand(fd, raw_msg);
	else if (cmd == "PING")
		return new PingCommand(fd, raw_msg);
	else if (cmd == "KICK")
		return new KickCommand(fd, raw_msg);
	else if (cmd == "INVITE")
		return new InviteCommand(fd, raw_msg);
	else if (cmd == "TOPIC")
		return new TopicCommand(fd, raw_msg);
	else if (cmd == "MODE")
		return new ModeCommand(fd, raw_msg);
	else
		return new UnknownCommand(fd, raw_msg);
}

MessageFactory::MessageFactory()
{
}

JoinCommand::JoinCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
						send(clientFD, joinMsg.c_str(), joinMsg.length(), 0);

						std::string topicMsg = ":server 332 " + nickname + " " + channelName + " :" + channel->getTopic() + "\r\n";
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
						send(clientFD, names.c_str(), names.length(), 0);

						std::string endNames = ":server 366 " + nickname + " " + channelName + " :End of /NAMES list\r\n";
						send(clientFD, endNames.c_str(), endNames.length(), 0);

						std::string notifyMsg = ":" + nickname + "!" + username + "@localhost JOIN " + channelName + "\r\n";
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
	parse();
}

void PrivmsgCommand::excute(Client *client)
{
	std::cout << "Executing PRIVMSG command for client FD: " << clientFD;
	if (getParamCount() >= 2)
	{
		std::string target = getParam(0);
		std::string message = getParam(1);
		std::cout << " Target: " << target << " Message: " << message;

		if (client != NULL)
		{

			if (!client->isClientRegistered())
			{
				std::string error = ":server 451 * :You have not registered\r\n";
				send(clientFD, error.c_str(), error.length(), 0);
				return;
			}

			std::string nickname = client->getNickname();
			std::string username = client->getUsername();

			if (!target.empty() && target[0] == '#')
			{

				Multiplexer *server = Multiplexer::getInstance();
				if (server)
				{
					Channel *channel = server->getChannel(target);
					if (channel && channel->isMember(client))
					{

						std::string channelMsg = ":" + nickname + "!" + username + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
						channel->broadcastToOthers(channelMsg, client);
					}
					else if (!channel)
					{

						std::string error = ":server 403 " + nickname + " " + target + " :No such channel\r\n";
						send(clientFD, error.c_str(), error.length(), 0);
					}
					else
					{

						std::string error = ":server 404 " + nickname + " " + target + " :Cannot send to channel\r\n";
						send(clientFD, error.c_str(), error.length(), 0);
					}
				}
			}
			else
			{

				std::string response = ":" + nickname + "!" + username + "@localhost PRIVMSG " + target + " :" + message + "\r\n";
				send(clientFD, response.c_str(), response.length(), 0);

				std::string notice = ":server NOTICE " + nickname + " :Message sent to " + target + " (echo mode)\r\n";
				send(clientFD, notice.c_str(), notice.length(), 0);
			}
		}
	}
	std::cout << std::endl;
}
NickCommand::NickCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication required";
				std::cout << std::endl;
				return;
			}

			std::string oldNick = client->getNickname();

			client->setNickname(nickname);

			if (oldNick.empty())
			{
				std::cout << " (First time setting nickname)";
			}
			else
			{

				std::string response = ":" + oldNick + " NICK :" + nickname + "\r\n";
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
	parse();
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
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication required";
				std::cout << std::endl;
				return;
			}

			client->setUsername(username);
			client->setRealname(realname);

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
	parse();
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
					send(clientFD, error.c_str(), error.length(), 0);
				}
				else
				{

					std::string error = ":server 442 " + client->getNickname() + " " + channelName + " :You're not on that channel\r\n";
					send(clientFD, error.c_str(), error.length(), 0);
				}
			}
		}
	}
	std::cout << std::endl;
}

QuitCommand::QuitCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	else
	{
		if (client != NULL)
		{
			std::string nickname = client->getNickname();

			std::string response = ":" + nickname + " QUIT :Client Quit\r\n";
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	std::cout << " - Client will be disconnected" << std::endl;
}

PingCommand::PingCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	else
	{
		if (client != NULL)
		{

			std::string response = ":server PONG server\r\n";
			send(clientFD, response.c_str(), response.length(), 0);
		}
	}
	std::cout << std::endl;
}

UnknownCommand::UnknownCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
}

void UnknownCommand::excute(Client *client)
{
	(void)client;
	std::cout << "Executing UNKNOWN command '" << command << "' for client FD: " << clientFD << std::endl;
}

PassCommand::PassCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
			}
			else
			{

				std::string response = ":server 464 * :Password incorrect\r\n";
				send(clientFD, response.c_str(), response.length(), 0);
				std::cout << " - Authentication failed";
			}
		}
	}
	std::cout << std::endl;
}

void testMessageParsing()
{
	std::cout << "\n=== Testing Message Parsing ===" << std::endl;

	Message *joinMsg = Message::createMessage(1, "JOIN #channel\r\n");
	if (joinMsg)
	{
		std::cout << "JOIN - Command: " << joinMsg->getCommand()
				  << ", Params: " << joinMsg->getParamCount()
				  << ", Is JOIN? " << (joinMsg->isCommand("join") ? "Yes" : "No") << std::endl;
		joinMsg->excute(NULL);
		delete joinMsg;
	}

	Message *privMsg = Message::createMessage(2, "PRIVMSG #channel :Hello world!\r\n");
	if (privMsg)
	{
		std::cout << "PRIVMSG - Command: " << privMsg->getCommand()
				  << ", Params: " << privMsg->getParamCount()
				  << ", Has trailing? " << (privMsg->hasTrailingParam() ? "Yes" : "No")
				  << ", Trailing: '" << privMsg->getTrailingParam() << "'" << std::endl;
		privMsg->excute(NULL);
		delete privMsg;
	}

	Message *nickMsg = Message::createMessage(3, "NICK testuser\r\n");
	if (nickMsg)
	{
		std::cout << "NICK - Command: " << nickMsg->getCommand()
				  << ", Params: " << nickMsg->getParamCount() << std::endl;
		nickMsg->excute(NULL);
		delete nickMsg;
	}

	Message *prefixMsg = Message::createMessage(4, ":server.com 001 user :Welcome\r\n");
	if (prefixMsg)
	{
		std::cout << "PREFIX - Command: " << prefixMsg->getCommand()
				  << ", Prefix: " << prefixMsg->getPrefix()
				  << ", Params: " << prefixMsg->getParamCount()
				  << ", Has prefix? " << (prefixMsg->hasPrefix() ? "Yes" : "No") << std::endl;
		prefixMsg->excute(NULL);
		delete prefixMsg;
	}

	std::cout << "=== End Testing ===" << std::endl;
}

KickCommand::KickCommand(int fd, std::string rawMessage) : Message(fd, rawMessage)
{
	parse();
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
					send(clientFD, response.c_str(), response.length(), 0);
				}
				else if (!server->isChannelOperator(clientFD, channelName))
				{
					std::string response = ":server 482 " + client->getNickname() + " " + channelName + " :You're not channel operator\r\n";
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
	parse();
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
	parse();
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
						send(clientFD, response.c_str(), response.length(), 0);
					}
					else
					{
						std::string response = ":server 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n";
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
	parse();
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
						send(clientFD, response.c_str(), response.length(), 0);
					}
				}
			}
			else
			{

				if (target == client->getNickname())
				{
					std::string response = ":server 221 " + client->getNickname() + " +\r\n";
					send(clientFD, response.c_str(), response.length(), 0);
				}
			}
		}
	}
	std::cout << std::endl;
}