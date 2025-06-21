# Reactor Design Pattern in FT_IRC

## What is the Reactor Design Pattern?

The Reactor design pattern is an architectural pattern used for handling service requests delivered concurrently to an application by one or more clients. It follows an event-driven approach where a central handler (the reactor) dispatches incoming requests to the appropriate service providers.

## Key Components in Our Implementation

### 1. Reactor (In `/reactor/reactor.hpp` and `/reactor/reactor.cpp`)

The central component that:
- Registers interest in specific events
- Detects when these events occur
- Dispatches handling of events to appropriate handlers

### 2. Event Demultiplexer (Using epoll)

The mechanism that blocks waiting for events on multiple file descriptors. In our IRC server, we're using Linux's `epoll` facility for this purpose.

### 3. Event Handlers (For client connections and messages)

Concrete handlers that implement the specific logic for processing different types of events (new connections, incoming messages, disconnections).

## Why Use the Reactor Pattern for an IRC Server?

### 1. Non-blocking I/O

The Reactor pattern allows our IRC server to handle multiple clients without dedicating a thread to each connection. This is crucial for IRC which may need to support hundreds of concurrent connections with minimal resource usage.

### 2. Event-Driven Architecture

IRC is inherently event-driven: clients connect/disconnect, send messages, join/leave channels - all representing discrete events that need handling. The Reactor pattern maps perfectly to this model.

### 3. Scalability

By using `epoll` as our event demultiplexer, we get an efficient O(1) mechanism for monitoring thousands of file descriptors, making the server highly scalable.

### 4. Separation of Concerns

The pattern cleanly separates event detection (the reactor) from event handling (the handlers), making the code more maintainable and easier to extend with new features.

## How Our Implementation Works

1. **Initialization**: The Reactor creates an epoll instance and registers the server socket
   ```cpp
   // In Reactor constructor
   epoll_fd = epoll_create1(0);
   event.events = EPOLLIN;
   event.data.fd = serverSocket.getServerFd();
   epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket.getServerFd(), &event);
   ```

2. **Event Loop**: The Reactor runs a continuous loop calling `epoll_wait()` to detect events
   ```cpp
   // In Reactor::run()
   while (running) {
       int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
       for (int i = 0; i < nfds; i++) {
           // Handle events
       }
   }
   ```

3. **Event Dispatch**: When events occur, the appropriate handlers are called
   ```cpp
   // Example dispatcher logic
   if (events[i].data.fd == serverSocket.getServerFd()) {
       // New client connection
       handleNewConnection();
   } else {
       // Client data available
       handleClientData(events[i].data.fd);
   }
   ```

## Reactor vs Other Patterns

### Compared to Thread-Per-Connection

- **Reactor**: Single-threaded, event-driven, efficient for many connections
- **Thread-Per-Connection**: One thread per client, simpler code but less scalable

### Compared to Proactor

- **Reactor**: Handlers react to completed events, synchronous
- **Proactor**: Handlers initiate operations, asynchronous

## Resources to Learn More

- [Pattern-Oriented Software Architecture, Volume 2](https://www.amazon.com/Pattern-Oriented-Software-Architecture-Concurrent-Networked/dp/0471606952) - Contains detailed discussion of the Reactor pattern
- [ACE (Adaptive Communication Environment)](https://www.dre.vanderbilt.edu/~schmidt/ACE.html) - A C++ framework that implements the Reactor pattern
- [The C10K problem](http://www.kegel.com/c10k.html) - Discussion of handling many simultaneous connections
