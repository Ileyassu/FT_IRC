#include "Channel.hpp"
#include "Client.hpp"
#include <algorithm>
#include <sys/socket.h>

Channel::Channel(const std::string &channelName)
	: name(channelName), inviteOnly(false), topicRestricted(true), userLimit(0)
{
	topic = "Welcome to " + channelName;
}

Channel::~Channel()
{
}

bool Channel::addMember(Client *client)
{
	if (!client || isMember(client))
		return false;

	bool isFirstMember = members.empty();

	members.insert(client);

	if (isFirstMember)
		operators.insert(client);

	return true;
}

bool Channel::removeMember(Client *client)
{
	if (!client || !isMember(client))
		return false;

	members.erase(client);
	operators.erase(client);
	return true;
}

bool Channel::isMember(Client *client) const
{
	return members.find(client) != members.end();
}

bool Channel::isOperator(Client *client) const
{
	return operators.find(client) != operators.end();
}

void Channel::addOperator(Client *client)
{
	if (isMember(client))
		operators.insert(client);
}

void Channel::removeOperator(Client *client)
{
	operators.erase(client);
}

void Channel::broadcast(const std::string &message, Client *sender)
{
	for (std::set<Client *>::iterator it = members.begin(); it != members.end(); ++it)
	{
		Client *member = *it;
		if (member && member != sender)
		{
			send(member->getSocketFd(), message.c_str(), message.length(), 0);
		}
	}
}

void Channel::broadcastToOthers(const std::string &message, Client *sender)
{
	for (std::set<Client *>::iterator it = members.begin(); it != members.end(); ++it)
	{
		Client *member = *it;
		if (member && member != sender)
		{
			send(member->getSocketFd(), message.c_str(), message.length(), 0);
		}
	}
}

const std::string &Channel::getName() const
{
	return name;
}

const std::string &Channel::getTopic() const
{
	return topic;
}

void Channel::setTopic(const std::string &newTopic)
{
	topic = newTopic;
}

size_t Channel::getMemberCount() const
{
	return members.size();
}

std::vector<Client *> Channel::getMembers() const
{
	std::vector<Client *> memberList;
	for (std::set<Client *>::const_iterator it = members.begin(); it != members.end(); ++it)
	{
		memberList.push_back(*it);
	}
	return memberList;
}

bool Channel::isInviteOnly() const
{
	return inviteOnly;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	this->inviteOnly = inviteOnly;
}

bool Channel::isTopicRestricted() const
{
	return topicRestricted;
}

void Channel::setTopicRestricted(bool restricted)
{
	this->topicRestricted = restricted;
}

const std::string &Channel::getKey() const
{
	return key;
}

void Channel::setKey(const std::string &password)
{
	key = password;
}

int Channel::getUserLimit() const
{
	return userLimit;
}

void Channel::setUserLimit(int limit)
{
	userLimit = limit;
}
