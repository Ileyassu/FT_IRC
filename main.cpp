#include "lib_irc/lib.hpp"
#include "IRC_msg_protocol/Message.hpp"
#include <cstdlib>

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	if (port <= 0 || port > 65535)
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
		std::cout << "Server is listening on port " << port << " with FD = " << obj.getServer().getServerFd() << std::endl;
		obj.event_loop();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}