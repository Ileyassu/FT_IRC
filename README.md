# 🚀 FT_IRC: The Socket Symphony Orchestra 🎼

Welcome to **FT_IRC** - where networking meets artistry! This isn't just another IRC server; it's a masterpiece of socket programming, event-driven architecture, and elegant design patterns all rolled into one epic C++98 adventure.

## 🎭 What's This All About?

Ever wondered how chat applications work under the hood? How thousands of users can chat simultaneously without the server breaking a sweat? Well, grab some popcorn because we're about to dive into the fascinating world of network programming!

## 📡 Chapter 1: The Magic of Sockets

### What Are Sockets? 🧙‍♂️

Think of sockets as magical portals that allow programs to talk to each other across networks. Just like how you might shout across a canyon and hear an echo, sockets let your program "shout" data across the internet and receive responses.

```
Your IRC Client  <---[Socket Magic]---> Our IRC Server
     "Hello!"                              "Hello back!"
```

### How Do They Work? ⚡

1. **Server Socket Creation**: We create a listening socket (like opening a shop)
2. **Binding**: We tell the socket which address and port to listen on (like putting up a shop sign)
3. **Listening**: The socket waits for clients to connect (like waiting for customers)
4. **Accepting**: When a client knocks, we accept the connection (like welcoming a customer)
5. **Communication**: Data flows back and forth (like having a conversation)

In our code, this magic happens in the `Server` class within the `multiplexing/server.cpp` file!

## 🎪 Chapter 2: The Epoll Circus - Why It's the Greatest Show on Earth

### The Old Days: select() and poll() 😴

Imagine you're a waiter at a restaurant with 1000 tables. The old way (`select()` and `poll()`) is like walking to EVERY table to ask "Do you need anything?" even if 999 of them are just sitting quietly. Exhausting, right?

### Enter Epoll: The Superhero! 🦸‍♂️

`epoll` is like having a magical bell system where tables only ring when they need something. You just sit back, relax, and respond only when someone actually needs attention!

#### Why Epoll Rocks Our Socks Off:

1. **🚀 Scalability**: Handles thousands of connections without breaking a sweat
2. **⚡ Performance**: O(1) complexity for adding/removing file descriptors
3. **🧠 Smart Notifications**: Only tells you about active connections
4. **💾 Memory Efficient**: Doesn't copy file descriptor sets around

```cpp
// Look at this beauty in multiplexing.cpp!
epoll_fd = epoll_create1(0);  // Create our magical event notifier
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event);  // Add a socket to watch
int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);  // Wait for magic to happen!
```

## 🎯 Chapter 3: The Reactor Pattern - Our Architectural Masterpiece

### What's the Reactor Pattern? 🔄

The Reactor pattern is like having a super-efficient party host who:
1. Watches the door for new guests (new connections)
2. Listens for guests who want to say something (incoming data)
3. Responds to guests who are leaving (disconnections)
4. Handles any party emergencies (errors)

All without ever getting overwhelmed or missing anyone!

### Our Reactor Implementation 🏗️

```cpp
void Multiplexer::event_loop()
{
    while (!server_shutdown)
    {
        // Wait for events (like a bouncer watching the door)
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < nfds; i++)
        {
            if (events[i].data.fd == serverFd.getServerFd())
            {
                handleNewConnection();  // "Welcome to the party!"
            }
            else
            {
                if (events[i].events & EPOLLIN)
                    handleRead(clientFd);    // "What did you say?"
                
                if (events[i].events & EPOLLOUT)
                    handleWrite(clientFd);   // "Here's my response!"
                
                if (events[i].events & (EPOLLERR | EPOLLHUP))
                    handleError(clientFd);   // "Oh no, someone left!"
            }
        }
    }
}
```

#### The Gang's All Here:

- **🎭 Multiplexer**: The main event coordinator
- **🏠 Server**: The venue that hosts everything
- **👥 Client**: Individual party guests
- **📡 Channel**: Group chat rooms where the fun happens
- **💬 Message**: The conversations flying around

## 🏛️ Chapter 4: User Management - The VIP System

### The Client Lifecycle Journey 🎢

Our user management is like running an exclusive club with multiple membership levels:

```cpp
enum ClientState
{
    UNREGISTERED,    // 🚪 "Who goes there?"
    NICK_SET,        // 👋 "Hi, I'm Bob!"
    USER_SET,        // 📝 "Here are my details"
    REGISTERED,      // ✅ "Welcome to the club!"
    AUTH             // 🔐 "Password accepted!"
}
```

### The Authentication Dance 💃

1. **Password Check**: `PASS secret_password`
   - Like showing your ID at the club entrance
   
2. **Nickname Setting**: `NICK CoolUser123`
   - "What should we call you?"
   
3. **User Details**: `USER username hostname servername :Real Name`
   - "Tell us about yourself!"

4. **Full Registration**: Once both NICK and USER are set
   - "Welcome to the IRC family! 🎉"

### The Command Factory Pattern 🏭

We use a elegant factory pattern for handling IRC commands:

```cpp
Message *Message::createMessage(int fd, const std::string &raw_msg)
{
    // Parse the command type
    if (cmd == "JOIN")        return new JoinCommand(fd, raw_msg);
    else if (cmd == "PRIVMSG") return new PrivmsgCommand(fd, raw_msg);
    else if (cmd == "NICK")    return new NickCommand(fd, raw_msg);
    // ... and many more!
}
```

Each command is like a specialized worker who knows exactly how to handle their job!

## 🏗️ Architecture Overview - The Big Picture

```
    📱 IRC Clients
         |
         | (TCP Sockets)
         ▼
    🏰 Server Socket (Listening)
         |
         ▼
    🎪 Multiplexer (Event Loop)
    ├── 🔍 Epoll (Event Detection)
    ├── 👥 Client Manager (User Handling)
    ├── 📡 Channel Manager (Room Management)
    └── 💬 Message Factory (Command Processing)
```

### Key Components:

- **🎭 Multiplexer Class**: The conductor of our socket orchestra
- **🏠 Server Class**: Creates and manages the main listening socket
- **👤 Client Class**: Represents each connected user with their state
- **📡 Channel Class**: Manages chat rooms and member lists
- **💬 Message Classes**: Handle different IRC commands (JOIN, PRIVMSG, etc.)

## 🚀 Getting Started - Join the Fun!

### Building the Server 🔨

```bash
# Compile the masterpiece
make

# Clean up if needed
make clean

# Start fresh
make re
```

### Running the Server 🎬

```bash
# Start your IRC empire!
./ircserv <port> <password>

# Example:
./ircserv 6667 mysecretpassword
```

### Connecting as a Client 📱

```bash
# Using netcat for testing
nc localhost 6667

# Or use any IRC client like:
# - HexChat
# - IRCCloud
# - WeeChat
```

### IRC Commands You Can Use 💬

```irc
PASS mysecretpassword     # Authenticate
NICK YourNickname         # Set your nickname
USER username 0 * :Real   # Set user info
JOIN #general             # Join a channel
PRIVMSG #general :Hello!  # Send a message
PART #general             # Leave a channel
QUIT :Goodbye!            # Disconnect
```

## 🎨 Design Patterns We Love

### 1. **Reactor Pattern** 🔄
- Single-threaded event-driven architecture
- Efficiently handles multiple clients
- Non-blocking I/O operations

### 2. **Factory Pattern** 🏭
- `Message::createMessage()` creates appropriate command objects
- Clean separation of command parsing and execution

### 3. **Singleton Pattern** 👑
- `Multiplexer::getInstance()` ensures single server instance
- Global access to server functionality

### 4. **Command Pattern** 📋
- Each IRC command is its own class
- Encapsulates command execution logic

## 🌟 What Makes This Special?

✨ **Asynchronous Magic**: Thanks to epoll, we handle thousands of users simultaneously
🎯 **Clean Architecture**: Each component has a single responsibility  
🚀 **Performance**: O(1) event handling with minimal memory footprint
🔒 **Robust**: Proper error handling and graceful shutdowns
🎨 **Extensible**: Easy to add new IRC commands and features

## 🎯 Technical Highlights

- **C++98 Compliant**: Old school cool with modern techniques
- **Non-blocking Sockets**: Never freeze, always responsive
- **Memory Management**: Proper cleanup of clients and channels
- **Protocol Compliance**: Real IRC protocol implementation
- **Signal Handling**: Graceful shutdown with Ctrl+C

## 🎉 Conclusion

This IRC server is more than just code - it's a symphony of networking concepts, design patterns, and careful engineering. From the humble socket to the mighty epoll, from individual clients to bustling channels, every piece works in harmony to create a robust, scalable chat server.

Whether you're here to learn about network programming, understand design patterns, or just appreciate some well-crafted C++, we hope this journey through our IRC server has been as exciting for you as it was for us to build!

Now go forth and chat! 💬✨

---

*Made with ❤️ and lots of ☕ by the FT_IRC team*

## 📚 Want to Learn More?

- **Sockets**: `man 2 socket`, `man 2 bind`, `man 2 listen`
- **Epoll**: `man 7 epoll`, `man 2 epoll_create`, `man 2 epoll_wait`
- **IRC Protocol**: RFC 1459, RFC 2812
- **Design Patterns**: "Design Patterns" by Gang of Four
- **Network Programming**: "Unix Network Programming" by W. Richard Stevens
