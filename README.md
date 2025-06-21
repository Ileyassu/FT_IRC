# FT_IRC

A simple IRC (Internet Relay Chat) server implementation in C++.

## Project Overview

This project implements a basic IRC server that handles client connections using socket programming and multiplexing for handling multiple clients simultaneously.

## Project Structure

- `main.cpp`: Entry point that initializes the server
- `client.cpp`: Client implementation for testing the server
- `lib_irc/lib.hpp`: Common header file with all necessary includes
- `multiplexing/`: Directory containing server and multiplexing implementation
  - `server.hpp/cpp`: Server class that handles socket creation and binding
  - `multiplexing.hpp`: Class for handling multiple client connections
- `reactor/`: Implementation of the Reactor design pattern
  - `reactor.hpp/cpp`: Core reactor implementation that manages the event loop
  - `README.md`: Detailed explanation of the Reactor design pattern

## Socket Programming Concepts

### What is a Socket?

A socket is an endpoint for sending or receiving data across a computer network. It's a combination of:
- IP address (which computer)
- Port number (which program on that computer)
- Protocol (how to communicate)

### Steps in Creating a Server Socket

1. **Creating a socket**: Using the `socket()` function
   ```cpp
   int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
   ```
   - `AF_INET`: IPv4 Internet protocols
   - `SOCK_STREAM`: TCP socket (connection-oriented)
   - `0`: Default protocol for the socket type

2. **Filling a socket address (sockaddr)**: 
   ```cpp
   sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(8080);
   serverAddress.sin_addr.s_addr = INADDR_ANY;
   ```
   - `sockaddr_in`: Structure for IPv4 socket address
   - `sin_family`: Address family (AF_INET for IPv4)
   - `sin_port`: Port number in network byte order
   - `sin_addr.s_addr`: IP address (INADDR_ANY means "any address")

3. **Binding the socket**:
   ```cpp
   bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
   ```
   - Associates the socket with a specific address and port
   - Necessary for the server to listen on a specific port

4. **Listening for connections**:
   ```cpp
   listen(serverSocket, 5);
   ```
   - Marks the socket as passive (able to accept connections)
   - 5 is the backlog (how many pending connections to queue)

5. **Accepting connections**:
   ```cpp
   int clientSocket = accept(serverSocket, nullptr, nullptr);
   ```
   - Creates a new socket for each client connection
   - Returns a file descriptor for communication with that client

## Why We Follow This Structure

- **Socket Creation**: Establishes an endpoint for communication
- **sockaddr Configuration**: Defines where and how clients should connect
- **Binding**: Associates the socket with a specific network interface and port
- **Listening/Accepting**: Allows the server to wait for and handle client connections

## Multiplexing & Reactor Pattern

This project uses I/O multiplexing to handle multiple client connections simultaneously without using multiple threads or processes. This is implemented using the Reactor design pattern and Linux's `epoll` facility.

### Reactor Design Pattern

We've implemented the IRC server using the Reactor architectural pattern, which:

- Uses event-driven programming to handle concurrent client connections efficiently
- Centralizes event handling through a dispatcher (the reactor)
- Separates event detection from event handling for better code organization

The Reactor pattern is ideal for IRC servers because:
1. It efficiently handles many concurrent connections with minimal resources
2. Maps well to IRC's inherently event-driven nature (connects, messages, joins, etc.)
3. Provides excellent scalability through non-blocking I/O

For detailed information about our Reactor implementation, see the [Reactor documentation](/home/ilyas/Desktop/FT_IRC/reactor/README.md).

## Building and Running

```bash
make
./irc
```

## Resources for Learning More

1. **Beej's Guide to Network Programming**: https://beej.us/guide/bgnet/ 
   An excellent and approachable guide to socket programming.

2. **The Linux Programming Interface** by Michael Kerrisk:
   Comprehensive reference on Linux system calls, including socket programming.

3. **RFC 1459**: https://datatracker.ietf.org/doc/html/rfc1459
   The original IRC protocol specification.

4. **POSIX Socket API Reference**: https://pubs.opengroup.org/onlinepubs/9699919799/
   Official documentation for socket functions.

5. **Unix Network Programming** by W. Richard Stevens:
   Classic book on network programming in Unix environments.

## Next Steps

- Complete server implementation with proper event handling
- Implement IRC protocol message parsing
- Add channel management and user authentication
