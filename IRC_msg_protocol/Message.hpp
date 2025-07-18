#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <vector>
#include <string>
#include <iostream>
#include <map>

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
	Message(/* args */);
	Message(int fd, std::string rawMessaeg);
	virtual ~Message();

	virtual bool parse();
	virtual void excute(Client *Client) = 0;

	// getters

	int getClientFd() const;
	const std::string &getCommand() const;
	const std::vector<std::string> &getParams() const;
	const std::string &getPrefix() const;
	const std::string &getRawMessage() const;

	// utils

	bool hasPrefix() const;
	size_t getParamCount() const;
	std::string getParam(size_t index) const;

	static Message *createMessage(int fd, const std::string &raw_msg);
};

class JoinCommand : Message
{
private:
	/* data */
public:
	JoinCommand(int fd, std::string rawMessage);
	void excute(Client *Client);

	// getters
};

class PrivmsgCommand : Message
{
private:
	/* data */
public:
	PrivmsgCommand(int fd, std::string rawMessage);
	void excute(Client *Client);

	// getters
};

class MessageFactory {
public:
    static Message* createMessage(int fd, const std::string& raw_msg);
    
private:
    // Map of command strings to factory functions
    static std::map<std::string, Message*(*)(int, const std::string&)> message_creators;
    
    // Factory functions for each message type
    static Message* createJoinMessage(int fd, const std::string& raw_msg);
    static Message* createPrivateMessage(int fd, const std::string& raw_msg);
    static Message* createNickMessage(int fd, const std::string& raw_msg);
    static Message* createQuitMessage(int fd, const std::string& raw_msg);
};

#endif // !MESSAGE_HPP