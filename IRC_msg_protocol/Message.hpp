#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <cctype>

class Client;
class Message
{
protected:
	int clientFD;
	std::string msg;
	std::string command;
	std::string prefix;
	std::vector<std::string> params;

public:
	Message();
	Message(int fd, std::string rawMessaeg);
	virtual ~Message();

	virtual bool parse();
	virtual void excute(Client *Client) = 0;

	int getClientFd() const;
	const std::string &getCommand() const;
	const std::vector<std::string> &getParams() const;
	const std::string &getPrefix() const;
	const std::string &getRawMessage() const;

	bool hasPrefix() const;
	size_t getParamCount() const;
	std::string getParam(size_t index) const;

	bool isCommand(const std::string &cmd) const;
	std::string getTrailingParam() const;
	bool hasTrailingParam() const;

	static Message *createMessage(int fd, const std::string &raw_msg);
};

class JoinCommand : public Message
{
private:
	
public:
	JoinCommand(int fd, std::string rawMessage);
	void excute(Client *Client);

};

class PrivmsgCommand : public Message
{
private:
	
public:
	PrivmsgCommand(int fd, std::string rawMessage);
	void excute(Client *Client);

};

class NickCommand : public Message
{
public:
	NickCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class UserCommand : public Message
{
public:
	UserCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class PartCommand : public Message
{
public:
	PartCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class QuitCommand : public Message
{
public:
	QuitCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class PingCommand : public Message
{
public:
	PingCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class UnknownCommand : public Message
{
public:
	UnknownCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class PassCommand : public Message
{
public:
	PassCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class KickCommand : public Message
{
public:
	KickCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class InviteCommand : public Message
{
public:
	InviteCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class TopicCommand : public Message
{
public:
	TopicCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class ModeCommand : public Message
{
public:
	ModeCommand(int fd, std::string rawMessage);
	void excute(Client *Client);
};

class MessageFactory
{
public:
	MessageFactory();
	static Message *createMessage(int fd, const std::string &raw_msg);
};

void testMessageParsing();

#endif