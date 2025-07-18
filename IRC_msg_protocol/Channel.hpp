#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <vector>

class Client;

class Channel
{
private:
	std::string name;
	std::string topic;
	std::set<Client *> members;
	std::set<Client *> operators;
	bool inviteOnly;
	bool topicRestricted;
	std::string key;
	int userLimit;

public:
	Channel(const std::string &channelName);
	~Channel();

	bool addMember(Client *client);
	bool removeMember(Client *client);
	bool isMember(Client *client) const;
	bool isOperator(Client *client) const;
	void addOperator(Client *client);
	void removeOperator(Client *client);

	void broadcast(const std::string &message, Client *sender = NULL);
	void broadcastToOthers(const std::string &message, Client *sender);

	const std::string &getName() const;
	const std::string &getTopic() const;
	void setTopic(const std::string &newTopic);
	size_t getMemberCount() const;
	std::vector<Client *> getMembers() const;

	bool isInviteOnly() const;
	void setInviteOnly(bool inviteOnly);
	bool isTopicRestricted() const;
	void setTopicRestricted(bool restricted);
	const std::string &getKey() const;
	void setKey(const std::string &password);
	int getUserLimit() const;
	void setUserLimit(int limit);
};

#endif