# tcp_server - Multi-threaded TCP Server

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Construction](#construction)
- [Server Lifecycle Management](#server-lifecycle-management)
- [Server Configuration](#server-configuration)
- [Server Status and Statistics](#server-status-and-statistics)
- [Signal Events](#signal-events)
- [Connection Factory Pattern](#connection-factory-pattern)
- [Threading Model](#threading-model)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)
- [Error Handling](#error-handling)
- [Performance Considerations](#performance-considerations)
- [Platform-Specific Notes](#platform-specific-notes)
- [See Also](#see-also)

---

## Overview

The `tcp_server` class provides a robust, multi-threaded TCP server implementation that accepts client connections and dispatches them to worker threads for processing. It manages a thread pool, connection queue, and provides comprehensive lifecycle management with graceful shutdown support.

**Key Features:**
- **Multi-threaded Architecture** - Separate acceptor thread and configurable worker thread pool
- **Dynamic Thread Pooling** - Automatically scales worker threads based on load (up to max_threads)
- **Connection Queue Management** - Configurable queue size with overflow handling
- **Signal-based Events** - Real-time notifications for server and connection events
- **Graceful Shutdown** - Configurable timeout for clean connection termination
- **Statistics & Monitoring** - Runtime metrics for active connections, uptime, and throughput
- **Exception Safety** - Robust error handling with virtual hooks for custom handling
- **Connection Lifecycle** - Automatic management of tcp_server_connection instances

**Architecture:**
```
                                    tcp_server
                                        |
                    +-------------------+-------------------+
                    |                                       |
              Acceptor Thread                      Worker Thread Pool
                    |                                       |
           (accepts connections)                   (processes connections)
                    |                                       |
                    v                                       v
            Connection Queue <---------------------------> Active Connections
                (FIFO)                                   (tracked & cleaned)
```

**Include:** `<fb/tcp_server.h>`
**Namespace:** `fb`
**Platform Support:** Windows, macOS, Linux, BSD

---

## Quick Start

### Basic Server Example

```cpp
#include <fb/fb_net.h>
using namespace fb;

// Define your connection handler
class EchoConnection : public tcp_server_connection {
public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        std::string buffer;
        while (!stop_requested()) {
            int received = socket().receive(buffer, 1024);
            if (received <= 0) break;
            socket().send(buffer);  // Echo back
        }
    }
};

int main() {
    fb::initialize_library();

    // Create and bind server socket
    server_socket server_sock(socket_address::IPv4);
    server_sock.bind(socket_address("0.0.0.0", 8080));
    server_sock.listen(50);

    // Connection factory
    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    // Create and start server
    tcp_server server(std::move(server_sock), factory);
    server.start();

    std::cout << "Server running. Press Enter to stop..." << std::endl;
    std::cin.get();

    server.stop();  // Graceful shutdown

    fb::cleanup_library();
    return 0;
}
```

---

## Construction

### Default Constructor

```cpp
tcp_server();
```

Constructs an idle server with default limits. The server is in an unconfigured state and requires calling `set_server_socket()` and `set_connection_factory()` before starting.

**Default Configuration:**
- `max_threads` = 100
- `max_queued` = 100
- `connection_timeout` = 30000 ms
- `idle_timeout` = 0 ms (disabled)

**Example:**
```cpp
tcp_server server;
server.set_server_socket(std::move(my_socket));
server.set_connection_factory(my_factory);
server.start();
```

---

### Full Constructor

```cpp
tcp_server(server_socket server_socket,
          connection_factory connection_factory,
          std::size_t max_threads = 0,
          std::size_t max_queued = 0);
```

Constructs and fully configures the server ready for `start()`.

**Parameters:**
- `server_socket` - Listening socket that must already be bound and listening
- `connection_factory` - Factory function to create tcp_server_connection instances
- `max_threads` - Maximum number of worker threads (0 = use default 100)
- `max_queued` - Maximum queued connections (0 = use default 100)

**Example:**
```cpp
server_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8080));
sock.listen(50);

auto factory = [](tcp_client socket, const socket_address& addr) {
    return std::make_unique<MyConnection>(std::move(socket), addr);
};

tcp_server server(std::move(sock), factory, 50, 200);  // Ready to start
```

---

### Move Semantics

```cpp
tcp_server(tcp_server&& other) noexcept;
tcp_server& operator=(tcp_server&& other) noexcept;
```

The server is **move-only** (no copy construction or assignment). Move operations transfer configuration and state.

**Behavior:**
- Moved-from server becomes unconfigured
- Active servers cannot be moved (stop first)
- Worker threads are not transferred (cannot move threads)

**Example:**
```cpp
tcp_server create_server() {
    server_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 8080));
    sock.listen();

    auto factory = [](tcp_client s, const socket_address& a) {
        return std::make_unique<MyConnection>(std::move(s), a);
    };

    return tcp_server(std::move(sock), factory);  // Move construction
}

tcp_server server = create_server();  // Moved into server
```

---

### Connection Factory Type

```cpp
using connection_factory = std::function<
    std::unique_ptr<tcp_server_connection>(tcp_client, const socket_address&)
>;
```

Factory function signature that creates connection handler instances.

**Parameters:**
- `tcp_client` - Accepted socket for the new connection
- `const socket_address&` - Client's remote address

**Returns:** `std::unique_ptr<tcp_server_connection>` (or derived class)

**Factory can return `nullptr`** to reject a connection without processing.

---

## Server Lifecycle Management

### start()

```cpp
void start();
```

Starts accepting client connections and dispatching work to the thread pool.

**Behavior:**
1. Validates configuration (socket and factory must be set)
2. Calls virtual `on_server_starting()` hook
3. Starts acceptor thread
4. Starts initial worker threads (min 4, up to max_threads)
5. Emits `onServerStarted` signal
6. Sets `is_running()` to `true`

**Throws:**
- `std::runtime_error` - If already running or configuration incomplete
- ``std::system_error`` - If listening socket is invalid or closed

**Example:**
```cpp
tcp_server server(std::move(sock), factory);
server.start();  // Server is now accepting connections
std::cout << "Server running with " << server.thread_count() << " threads\n";
```

---

### stop()

```cpp
void stop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(5000));
```

Stops accepting connections and shuts down worker threads gracefully.

**Parameters:**
- `timeout` - Maximum time to wait for threads and connections to finish (default: 5000ms)

**Behavior:**
1. Sets internal stop flags
2. Emits `onServerStopping` signal
3. Calls virtual `on_server_stopping()` hook
4. Closes server socket to stop accepting
5. Signals all active connections to stop
6. Wakes up all worker threads
7. Joins acceptor thread
8. Joins worker threads (detaches if timeout exceeded)
9. Cleans up remaining connections
10. Emits `onServerStopped` signal

**Throws:**
- `std::runtime_error` - If server is not currently running

**Example:**
```cpp
server.start();
// ... server is running ...
server.stop(std::chrono::seconds(10));  // 10 second graceful shutdown
```

---

### is_running()

```cpp
bool is_running() const;
```

Queries whether the server is actively running.

**Returns:** `true` if acceptor thread is running and server has not been stopped

**Example:**
```cpp
if (server.is_running()) {
    std::cout << "Server is active with " << server.active_connections()
              << " connections\n";
}
```

---

## Server Configuration

All configuration methods throw `std::runtime_error` if called while the server is running. Stop the server first to reconfigure.

### set_server_socket()

```cpp
void set_server_socket(server_socket server_socket);
```

Provides a listening socket to the server. Socket must already be bound and listening.

**Example:**
```cpp
server_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8080));
sock.listen(50);

tcp_server server;
server.set_server_socket(std::move(sock));
```

---

### set_connection_factory()

```cpp
void set_connection_factory(connection_factory factory);
```

Installs the factory used to create per-connection handlers.

**Example:**
```cpp
auto factory = [](tcp_client socket, const socket_address& addr) {
    std::cout << "New connection from " << addr.to_string() << "\n";
    return std::make_unique<MyConnection>(std::move(socket), addr);
};

server.set_connection_factory(factory);
```

---

### set_max_threads()

```cpp
void set_max_threads(std::size_t max_threads);
```

Configures the upper bound on worker threads. Server dynamically scales from 4 initial threads up to this maximum based on load.

**Parameters:**
- `max_threads` - Maximum worker count (0 = use default 100)

**Example:**
```cpp
server.set_max_threads(50);  // Allow up to 50 concurrent connections
```

---

### set_max_queued()

```cpp
void set_max_queued(std::size_t max_queued);
```

Sets the maximum number of pending connections waiting for a worker thread.

**Parameters:**
- `max_queued` - Queue capacity (0 = use default 100)

**Behavior:**
- When queue is full, new connections are **dropped** (not accepted)
- Prevents unbounded memory growth under heavy load

**Example:**
```cpp
server.set_max_queued(200);  // Allow 200 pending connections
```

---

### set_connection_timeout()

```cpp
void set_connection_timeout(const std::chrono::milliseconds& timeout);
```

Configures the per-connection I/O timeout applied to each tcp_server_connection.

**Parameters:**
- `timeout` - Duration applied to each connection (0 = no timeout)

**Example:**
```cpp
server.set_connection_timeout(std::chrono::seconds(30));  // 30s I/O timeout
```

---

### set_idle_timeout()

```cpp
void set_idle_timeout(const std::chrono::milliseconds& timeout);
```

Configures automatic shutdown of idle connections (currently not fully implemented in connection handling).

**Parameters:**
- `timeout` - Idle timeout; zero disables automatic disconnects

**Example:**
```cpp
server.set_idle_timeout(std::chrono::minutes(5));  // 5 minute idle timeout
```

---

## Server Status and Statistics

### server_socket()

```cpp
const server_socket& server_socket() const;
```

Accesses the listening socket.

**Returns:** Reference to the configured server_socket

**Throws:** `std::runtime_error` if no socket has been configured

**Example:**
```cpp
std::cout << "Server bound to: " << server.server_socket().address().to_string() << "\n";
```

---

### active_connections()

```cpp
std::size_t active_connections() const;
```

Gets the number of connections currently being serviced.

**Returns:** Count of active tcp_server_connection instances

**Thread-Safe:** Yes

**Example:**
```cpp
std::cout << "Active connections: " << server.active_connections() << "\n";
```

---

### thread_count()

```cpp
std::size_t thread_count() const;
```

Gets the current number of worker threads in the pool.

**Returns:** Size of the worker thread pool

**Thread-Safe:** Yes

**Example:**
```cpp
std::cout << "Worker threads: " << server.thread_count() << "\n";
```

---

### total_connections()

```cpp
std::uint64_t total_connections() const;
```

Retrieves the cumulative number of accepted clients since server start.

**Returns:** Total clients accepted (lifetime counter)

**Thread-Safe:** Yes (atomic)

**Example:**
```cpp
std::cout << "Total connections handled: " << server.total_connections() << "\n";
```

---

### queued_connections()

```cpp
std::size_t queued_connections() const;
```

Reports how many accepted connections are waiting for a worker thread.

**Returns:** Pending connection count in the queue

**Thread-Safe:** Yes

**Example:**
```cpp
if (server.queued_connections() > 50) {
    std::cout << "Warning: High connection queue depth!\n";
}
```

---

### uptime()

```cpp
std::chrono::steady_clock::duration uptime() const;
```

Measures how long the server has been running.

**Returns:** Duration since `start()` was called, or zero if not running

**Example:**
```cpp
auto uptime_seconds = std::chrono::duration_cast<std::chrono::seconds>(
    server.uptime()).count();
std::cout << "Server uptime: " << uptime_seconds << " seconds\n";
```

---

## Signal Events

The server emits signals for monitoring and observability. **All signals are emitted on acceptor/worker threads**, not the main thread. Ensure thread-safe handling.

### onServerStarted

```cpp
fb::signal<> onServerStarted;
```

Emitted when the server successfully starts and begins accepting connections.

**Example:**
```cpp
server.onServerStarted.connect([]() {
    std::cout << "[EVENT] Server started\n";
});
```

---

### onServerStopping

```cpp
fb::signal<> onServerStopping;
```

Emitted when the server begins the shutdown process.

**Example:**
```cpp
server.onServerStopping.connect([]() {
    std::cout << "[EVENT] Server stopping...\n";
});
```

---

### onServerStopped

```cpp
fb::signal<> onServerStopped;
```

Emitted when the server has completely stopped (after threads join).

**Example:**
```cpp
server.onServerStopped.connect([]() {
    std::cout << "[EVENT] Server stopped\n";
});
```

---

### onConnectionAccepted

```cpp
fb::signal<const socket_address&> onConnectionAccepted;
```

Emitted when a new connection is accepted (before factory is called).

**Parameters:**
- `const socket_address&` - Client's remote address

**Example:**
```cpp
server.onConnectionAccepted.connect([](const socket_address& addr) {
    std::cout << "[EVENT] Connection from: " << addr.to_string() << "\n";
});
```

---

### onConnectionClosed

```cpp
fb::signal<const socket_address&> onConnectionClosed;
```

Emitted when a connection closes.

**Parameters:**
- `const socket_address&` - Client's remote address

**Example:**
```cpp
server.onConnectionClosed.connect([](const socket_address& addr) {
    std::cout << "[EVENT] Disconnected: " << addr.to_string() << "\n";
});
```

---

### onActiveConnectionsChanged

```cpp
fb::signal<size_t> onActiveConnectionsChanged;
```

Emitted when the active connection count changes.

**Parameters:**
- `size_t` - New active connection count

**Example:**
```cpp
server.onActiveConnectionsChanged.connect([](size_t count) {
    std::cout << "[EVENT] Active connections: " << count << "\n";
});
```

---

### onException

```cpp
fb::signal<const std::exception&, const std::string&> onException;
```

Emitted when an exception occurs during server operation.

**Parameters:**
- `const std::exception&` - The exception that was thrown
- `const std::string&` - Context string (e.g., "acceptor_thread", "stop")

**Example:**
```cpp
server.onException.connect([](const std::exception& ex, const std::string& ctx) {
    std::cerr << "[ERROR] Exception in " << ctx << ": " << ex.what() << "\n";
});
```

---

## Connection Factory Pattern

The connection factory is the heart of server customization. It creates connection handler instances for each accepted client.

### Factory Signature

```cpp
std::unique_ptr<tcp_server_connection> factory(
    tcp_client socket,
    const socket_address& client_address
);
```

### Factory Patterns

#### 1. Simple Factory (Lambda)

```cpp
auto factory = [](tcp_client socket, const socket_address& addr) {
    return std::make_unique<MyConnection>(std::move(socket), addr);
};
```

#### 2. Factory with Configuration

```cpp
auto factory = [&config](tcp_client socket, const socket_address& addr) {
    auto conn = std::make_unique<MyConnection>(std::move(socket), addr);
    conn->set_config(config);  // Configure connection
    return conn;
};
```

#### 3. Connection Filtering

```cpp
auto factory = [](tcp_client socket, const socket_address& addr) {
    // Reject connections from specific IPs
    if (addr.host() == "192.168.1.100") {
        return std::unique_ptr<MyConnection>(nullptr);  // Reject
    }
    return std::make_unique<MyConnection>(std::move(socket), addr);
};
```

#### 4. Multiple Connection Types

```cpp
auto factory = [](tcp_client socket, const socket_address& addr) {
    // Route to different handlers based on port
    if (addr.port() == 8080) {
        return std::make_unique<HttpConnection>(std::move(socket), addr);
    } else {
        return std::make_unique<RawConnection>(std::move(socket), addr);
    }
};
```

#### 5. Factory Class

```cpp
class ConnectionFactory {
    Database& db;
public:
    ConnectionFactory(Database& database) : db(database) {}

    std::unique_ptr<tcp_server_connection> operator()(
        tcp_client socket, const socket_address& addr)
    {
        return std::make_unique<DatabaseConnection>(
            std::move(socket), addr, db);
    }
};

ConnectionFactory factory(my_database);
server.set_connection_factory(factory);
```

---

## Threading Model

### Architecture Overview

```
Main Thread                    Acceptor Thread              Worker Thread Pool
    |                                |                              |
start() --------------------------> accept() loop           wait for connections
    |                                |                              |
    |                          [new connection]                     |
    |                                |                              |
    |                        create handler ------------------------+
    |                                |                              |
    |                        queue connection                       |
    |                                |                              |
    |                        notify workers ----------------------> dequeue
    |                                |                              |
    |                        add thread if needed                run()
    |                                |                              |
    |                                |                         cleanup & repeat
```

### Thread Pools

#### Acceptor Thread
- **Count:** 1
- **Purpose:** Accept incoming connections
- **Behavior:**
  - Calls `server_socket::accept_connection()` with 1-second timeout
  - Creates connection handler via factory
  - Queues connection for worker processing
  - Adds worker threads dynamically if needed

#### Worker Thread Pool
- **Initial Count:** 4 threads (minimum)
- **Maximum Count:** Configurable (default 100)
- **Scaling:** Automatic based on queue depth
- **Behavior:**
  - Wait on condition variable for queued connections
  - Dequeue connection and call `connection->start()` which invokes `run()`
  - Add to active connections list for tracking
  - Clean up finished connections periodically

### Thread Scaling Algorithm

```cpp
// Add worker thread if:
if (connection_queue.size() > worker_threads.size() &&
    worker_threads.size() < max_threads) {
    worker_threads.emplace_back(worker_thread_proc);
}
```

**Scaling triggers when:**
1. Queue depth exceeds current thread count
2. Thread count is below max_threads
3. Called after each connection is queued

### Thread Safety

**Mutexes:**
- `m_queue_mutex` - Protects connection queue
- `m_threads_mutex` - Protects worker thread vector
- `m_connections_mutex` - Protects active connections list

**Atomics:**
- `m_running` - Server running state
- `m_should_stop` - Shutdown signal
- `m_total_connections` - Connection counter

**Condition Variables:**
- `m_queue_condition` - Wakes worker threads when connections are queued

---

## Complete Examples

### Example 1: Echo Server with Logging

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <chrono>

using namespace fb;

class EchoConnection : public tcp_server_connection {
public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        std::cout << "[" << client_address().to_string()
                  << "] Connected\n";

        std::string buffer;
        while (!stop_requested()) {
            try {
                int received = socket().receive(buffer, 4096);
                if (received <= 0) break;

                std::cout << "[" << client_address().to_string()
                          << "] Received " << received << " bytes\n";

                socket().send(buffer);
            } catch (const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
                break;
            }
        }

        std::cout << "[" << client_address().to_string()
                  << "] Disconnected\n";
    }
};

int main() {
    fb::initialize_library();

    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 8080));
    sock.listen(50);

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(sock), factory, 50, 100);

    // Attach signal handlers
    server.onServerStarted.connect([]() {
        std::cout << "Server started successfully\n";
    });

    server.onException.connect([](const std::exception& ex, const std::string& ctx) {
        std::cerr << "Exception in " << ctx << ": " << ex.what() << "\n";
    });

    server.start();

    std::cout << "Echo server listening on port 8080\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: HTTP Server with Statistics

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>

using namespace fb;

std::atomic<uint64_t> g_requests{0};

class HttpConnection : public tcp_server_connection {
public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        socket().set_receive_timeout(std::chrono::seconds(10));

        try {
            std::string request;
            socket().receive(request, 8192);

            g_requests++;

            std::ostringstream response;
            response << "HTTP/1.1 200 OK\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n\r\n"
                    << "<html><body>"
                    << "<h1>FB_NET Server</h1>"
                    << "<p>Request #" << g_requests << "</p>"
                    << "<p>Your IP: " << client_address().to_string() << "</p>"
                    << "</body></html>";

            socket().send(response.str());
        } catch (const std::exception& ex) {
            std::cerr << "HTTP error: " << ex.what() << "\n";
        }
    }
};

int main() {
    fb::initialize_library();

    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 8080));
    sock.listen(100);

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<HttpConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(sock), factory, 100, 500);
    server.start();

    std::cout << "HTTP server listening on http://localhost:8080\n";

    // Statistics thread
    std::atomic<bool> running{true};
    std::thread stats_thread([&]() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            auto uptime_sec = std::chrono::duration_cast<std::chrono::seconds>(
                server.uptime()).count();

            std::cout << "[STATS] Uptime: " << uptime_sec << "s"
                      << " | Active: " << server.active_connections()
                      << " | Total: " << server.total_connections()
                      << " | Requests: " << g_requests.load()
                      << " | Threads: " << server.thread_count() << "\n";
        }
    });

    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    running = false;
    server.stop(std::chrono::seconds(5));
    stats_thread.join();

    fb::cleanup_library();
    return 0;
}
```

---

### Example 3: Chat Server with Broadcasting

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>

using namespace fb;

// Global client list
std::vector<std::weak_ptr<tcp_client>> g_clients;
std::mutex g_clients_mutex;

void broadcast(const std::string& message, tcp_client* sender = nullptr) {
    std::lock_guard<std::mutex> lock(g_clients_mutex);

    // Remove expired weak_ptrs
    g_clients.erase(
        std::remove_if(g_clients.begin(), g_clients.end(),
            [](auto& wp) { return wp.expired(); }),
        g_clients.end()
    );

    // Broadcast to all active clients
    for (auto& weak_client : g_clients) {
        if (auto client = weak_client.lock()) {
            if (client.get() != sender) {
                try {
                    client->send(message + "\n");
                } catch (...) {
                    // Ignore send errors
                }
            }
        }
    }
}

class ChatConnection : public tcp_server_connection {
    std::shared_ptr<tcp_client> m_shared_socket;
    std::string m_nickname;

public:
    ChatConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        // Move socket to shared_ptr for broadcasting
        m_shared_socket = std::make_shared<tcp_client>(std::move(socket()));
    }

    void run() override {
        try {
            // Get nickname
            m_shared_socket->send("Enter nickname: ");
            m_shared_socket->receive(m_nickname, 256);
            m_nickname.erase(std::remove(m_nickname.begin(), m_nickname.end(), '\n'),
                           m_nickname.end());

            // Add to client list
            {
                std::lock_guard<std::mutex> lock(g_clients_mutex);
                g_clients.push_back(m_shared_socket);
            }

            broadcast("*** " + m_nickname + " joined the chat ***");

            // Message loop
            std::string message;
            while (!stop_requested()) {
                int received = m_shared_socket->receive(message, 1024);
                if (received <= 0) break;

                message.erase(std::remove(message.begin(), message.end(), '\n'),
                            message.end());

                if (!message.empty()) {
                    broadcast("<" + m_nickname + "> " + message, m_shared_socket.get());
                }
            }

            broadcast("*** " + m_nickname + " left the chat ***");

        } catch (const std::exception& ex) {
            std::cerr << "Chat error: " << ex.what() << "\n";
        }
    }
};

int main() {
    fb::initialize_library();

    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 9090));
    sock.listen(50);

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<ChatConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(sock), factory);
    server.start();

    std::cout << "Chat server listening on port 9090\n";
    std::cout << "Connect with: telnet localhost 9090\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 4: Custom Server with Virtual Hooks

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <fstream>

using namespace fb;

class LoggingServer : public tcp_server {
    std::ofstream m_log_file;

public:
    LoggingServer(server_socket sock, connection_factory factory)
        : tcp_server(std::move(sock), factory)
        , m_log_file("server.log", std::ios::app)
    {
        if (!m_log_file.is_open()) {
            throw std::runtime_error("Cannot open log file");
        }
    }

protected:
    void on_server_starting() override {
        m_log_file << "[" << timestamp() << "] Server starting...\n";
        m_log_file.flush();
    }

    void on_server_stopping() override {
        m_log_file << "[" << timestamp() << "] Server stopping...\n";
        m_log_file.flush();
    }

    void handle_exception(const std::exception& ex,
                         const std::string& context) noexcept override {
        m_log_file << "[" << timestamp() << "] ERROR in " << context
                   << ": " << ex.what() << "\n";
        m_log_file.flush();
        std::cerr << "Exception logged: " << ex.what() << "\n";
    }

private:
    std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        return buf;
    }
};

class EchoConnection : public tcp_server_connection {
public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        std::string buffer;
        while (!stop_requested()) {
            int received = socket().receive(buffer, 4096);
            if (received <= 0) break;
            socket().send(buffer);
        }
    }
};

int main() {
    fb::initialize_library();

    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 8080));
    sock.listen(50);

    auto factory = [](tcp_client socket, const socket_address& addr) {
        return std::make_unique<EchoConnection>(std::move(socket), addr);
    };

    LoggingServer server(std::move(sock), factory);
    server.start();

    std::cout << "Logging server running. Check server.log for activity.\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 5: Protocol Server with Connection Filtering

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <unordered_set>
#include <mutex>

using namespace fb;

// IP blacklist
std::unordered_set<std::string> g_blacklist;
std::mutex g_blacklist_mutex;

class ProtocolConnection : public tcp_server_connection {
public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        socket().set_receive_timeout(std::chrono::seconds(30));

        try {
            // Protocol handshake
            socket().send("PROTO v1.0\n");

            std::string handshake;
            socket().receive(handshake, 256);

            if (handshake.find("CLIENT v1.0") == std::string::npos) {
                socket().send("ERROR: Invalid protocol\n");
                return;
            }

            socket().send("OK\n");

            // Handle protocol messages
            std::string message;
            while (!stop_requested()) {
                int received = socket().receive(message, 4096);
                if (received <= 0) break;

                // Echo protocol: prepend "ECHO: " to received data
                socket().send("ECHO: " + message);
            }

        } catch (const `std::system_error` (std::errc::timed_out)&) {
            socket().send("ERROR: Timeout\n");
        } catch (const std::exception& ex) {
            std::cerr << "Protocol error: " << ex.what() << "\n";
        }
    }
};

int main() {
    fb::initialize_library();

    // Add some IPs to blacklist
    {
        std::lock_guard<std::mutex> lock(g_blacklist_mutex);
        g_blacklist.insert("192.168.1.100");
        g_blacklist.insert("10.0.0.50");
    }

    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 7777));
    sock.listen(50);

    // Factory with IP filtering
    auto factory = [](tcp_client socket, const socket_address& addr)
        -> std::unique_ptr<tcp_server_connection>
    {
        std::string client_ip = addr.host();

        // Check blacklist
        {
            std::lock_guard<std::mutex> lock(g_blacklist_mutex);
            if (g_blacklist.count(client_ip) > 0) {
                std::cout << "BLOCKED connection from blacklisted IP: "
                         << client_ip << "\n";
                return nullptr;  // Reject connection
            }
        }

        std::cout << "ACCEPTED connection from: " << addr.to_string() << "\n";
        return std::make_unique<ProtocolConnection>(std::move(socket), addr);
    };

    tcp_server server(std::move(sock), factory, 50, 200);
    server.start();

    std::cout << "Protocol server listening on port 7777\n";
    std::cout << "Blacklisted IPs are rejected\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Server Lifecycle Management

```cpp
class ServerManager {
    tcp_server m_server;
    std::atomic<bool> m_running{false};

public:
    void start(server_socket sock, tcp_server::connection_factory factory) {
        if (m_running) {
            throw std::runtime_error("Server already running");
        }

        m_server.set_server_socket(std::move(sock));
        m_server.set_connection_factory(factory);
        m_server.start();
        m_running = true;
    }

    void stop() {
        if (m_running) {
            m_server.stop(std::chrono::seconds(10));
            m_running = false;
        }
    }

    ~ServerManager() {
        stop();  // RAII cleanup
    }
};
```

---

### Pattern 2: Dynamic Configuration

```cpp
struct ServerConfig {
    std::string bind_address = "0.0.0.0";
    uint16_t port = 8080;
    size_t max_threads = 100;
    size_t max_queued = 200;
    std::chrono::milliseconds connection_timeout{30000};
};

tcp_server create_configured_server(
    const ServerConfig& config,
    tcp_server::connection_factory factory)
{
    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address(config.bind_address, config.port));
    sock.listen(50);

    tcp_server server(std::move(sock), factory,
                     config.max_threads, config.max_queued);
    server.set_connection_timeout(config.connection_timeout);

    return server;  // Move construction
}
```

---

### Pattern 3: Connection Pool Management

```cpp
class ConnectionPool {
    std::vector<std::weak_ptr<tcp_server_connection>> m_connections;
    mutable std::mutex m_mutex;

public:
    void add(std::shared_ptr<tcp_server_connection> conn) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connections.push_back(conn);
        cleanup();
    }

    size_t active_count() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::count_if(m_connections.begin(), m_connections.end(),
            [](const auto& wp) { return !wp.expired(); });
    }

    void stop_all() {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& wp : m_connections) {
            if (auto conn = wp.lock()) {
                conn->stop();
            }
        }
    }

private:
    void cleanup() {
        m_connections.erase(
            std::remove_if(m_connections.begin(), m_connections.end(),
                [](auto& wp) { return wp.expired(); }),
            m_connections.end()
        );
    }
};
```

---

### Pattern 4: Request Rate Limiting

```cpp
class RateLimiter {
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> m_requests;
    std::mutex m_mutex;
    size_t m_max_requests = 10;
    std::chrono::seconds m_window{60};

public:
    bool allow(const std::string& client_ip) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto now = std::chrono::steady_clock::now();
        auto& requests = m_requests[client_ip];

        // Remove old requests outside window
        while (!requests.empty() &&
               now - requests.front() > m_window) {
            requests.pop_front();
        }

        if (requests.size() >= m_max_requests) {
            return false;  // Rate limit exceeded
        }

        requests.push_back(now);
        return true;
    }
};

RateLimiter g_rate_limiter;

auto factory = [](tcp_client socket, const socket_address& addr)
    -> std::unique_ptr<tcp_server_connection>
{
    if (!g_rate_limiter.allow(addr.host())) {
        std::cout << "Rate limit exceeded for " << addr.host() << "\n";
        return nullptr;  // Reject
    }
    return std::make_unique<MyConnection>(std::move(socket), addr);
};
```

---

### Pattern 5: Graceful Shutdown with Timeout

```cpp
void graceful_shutdown(tcp_server& server,
                      std::chrono::milliseconds timeout = std::chrono::seconds(30))
{
    std::cout << "Shutting down server...\n";

    auto start = std::chrono::steady_clock::now();

    // Stop accepting new connections
    server.stop(timeout);

    auto elapsed = std::chrono::steady_clock::now() - start;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    std::cout << "Server stopped in " << elapsed_ms.count() << "ms\n";
    std::cout << "Final stats - Total connections: "
              << server.total_connections() << "\n";
}
```

---

## Error Handling

### Exception Types

The server throws the following exceptions:

**std::runtime_error:**
- Server already running when `start()` is called
- Server not running when `stop()` is called
- Configuration methods called while running
- Server not properly configured (no socket or factory)
- No server socket is set when accessing `server_socket()`

**`std::system_error`:**
- Server socket is closed or invalid when starting

**std::system_error and derived:**
- Network errors during accept operations (caught internally)
- Connection processing errors (caught and handled via `handle_exception()`)

---

### Virtual Exception Hook

```cpp
class MyServer : public tcp_server {
protected:
    void handle_exception(const std::exception& ex,
                         const std::string& context) noexcept override
    {
        if (dynamic_cast<const `std::system_error` (std::errc::timed_out)*>(&ex)) {
            // Timeouts are normal, ignore
            return;
        }

        // Log other exceptions
        std::cerr << "[" << timestamp() << "] ERROR in " << context
                  << ": " << ex.what() << "\n";

        // Could also:
        // - Send alert
        // - Increment error counter
        // - Trigger circuit breaker
    }
};
```

---

### Signal-based Error Handling

```cpp
tcp_server server(std::move(sock), factory);

server.onException.connect([](const std::exception& ex, const std::string& ctx) {
    // Log to file
    log_error(ctx, ex.what());

    // Send metrics
    metrics::increment("server.errors", {{"context", ctx}});

    // Alert if critical
    if (ctx == "acceptor_thread") {
        send_alert("Server acceptor thread error: " + std::string(ex.what()));
    }
});

server.start();
```

---

### Robust Error Handling Pattern

```cpp
try {
    server_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 8080));
    sock.listen(50);

    auto factory = [](tcp_client socket, const socket_address& addr) {
        try {
            return std::make_unique<MyConnection>(std::move(socket), addr);
        } catch (const std::exception& ex) {
            std::cerr << "Factory error: " << ex.what() << "\n";
            return std::unique_ptr<MyConnection>(nullptr);
        }
    };

    tcp_server server(std::move(sock), factory);

    server.onException.connect([](const std::exception& ex, const std::string& ctx) {
        std::cerr << "Server exception in " << ctx << ": " << ex.what() << "\n";
    });

    server.start();

    // Wait for signal
    std::cout << "Server running. Press Ctrl+C to stop.\n";
    wait_for_signal();

    server.stop(std::chrono::seconds(30));

} catch (const `std::system_error`& ex) {
    std::cerr << "Socket error: " << ex.what() << "\n";
    return 1;
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
}
```

---

## Performance Considerations

### Thread Pool Sizing

**Guidelines:**
- **CPU-bound tasks:** `max_threads` = number of CPU cores
- **I/O-bound tasks:** `max_threads` = 50-200 (higher for blocking I/O)
- **Mixed workload:** `max_threads` = 2-4 × CPU cores

```cpp
// CPU-bound (computation-heavy)
server.set_max_threads(std::thread::hardware_concurrency());

// I/O-bound (database queries, file I/O)
server.set_max_threads(100);

// High concurrency (simple request-response)
server.set_max_threads(200);
```

---

### Queue Depth Tuning

**Guidelines:**
- **Low latency:** Small queue (10-50) to prevent request queuing
- **High throughput:** Larger queue (200-1000) to buffer traffic spikes
- **Memory constrained:** Smaller queue to limit memory usage

```cpp
// Low latency server
server.set_max_queued(20);

// High throughput server
server.set_max_queued(500);

// Monitor queue depth
if (server.queued_connections() > 100) {
    std::cout << "Warning: High queue depth, consider increasing threads\n";
}
```

---

### Connection Timeout Configuration

```cpp
// Quick timeout for interactive protocols
server.set_connection_timeout(std::chrono::seconds(10));

// Longer timeout for batch processing
server.set_connection_timeout(std::chrono::minutes(5));

// No timeout for long-lived connections
server.set_connection_timeout(std::chrono::milliseconds(0));
```

---

### Backlog Size (listen queue)

```cpp
server_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8080));

// Small backlog for low-traffic servers
sock.listen(10);

// Large backlog for high-traffic servers (handle traffic bursts)
sock.listen(128);  // Typical maximum on many systems
```

---

### Zero-Copy and Buffer Management

```cpp
class OptimizedConnection : public tcp_server_connection {
    static constexpr size_t BUFFER_SIZE = 8192;
    char m_buffer[BUFFER_SIZE];

public:
    using tcp_server_connection::tcp_server_connection;

    void run() override {
        while (!stop_requested()) {
            // Reuse buffer, no allocations
            int received = socket().receive_bytes(m_buffer, BUFFER_SIZE);
            if (received <= 0) break;

            // Process without copying
            process_data(m_buffer, received);

            socket().send_bytes(m_buffer, received);
        }
    }

    void process_data(const char* data, size_t len) {
        // In-place processing
    }
};
```

---

### Monitoring and Metrics

```cpp
// Periodic metrics collection
void monitor_server(tcp_server& server) {
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        auto active = server.active_connections();
        auto queued = server.queued_connections();
        auto threads = server.thread_count();
        auto total = server.total_connections();

        // Calculate metrics
        double queue_utilization = queued / static_cast<double>(server.max_queued);
        double thread_utilization = threads / static_cast<double>(server.max_threads);

        // Log or send to monitoring system
        std::cout << "Active: " << active
                  << " | Queued: " << queued << " (" << (queue_utilization*100) << "%)"
                  << " | Threads: " << threads << " (" << (thread_utilization*100) << "%)"
                  << " | Total: " << total << "\n";

        // Alerts
        if (queue_utilization > 0.8) {
            std::cout << "WARNING: Queue nearly full, increase threads!\n";
        }
    }
}
```

---

## Platform-Specific Notes

### Windows

**Winsock Initialization:**
```cpp
// FB_NET handles this automatically via initialize_library()
fb::initialize_library();  // Calls WSAStartup()
// ... use server ...
fb::cleanup_library();      // Calls WSACleanup()
```

**Thread Limit:** Windows supports thousands of threads, but practical limit depends on available memory.

**Listen Backlog:** Windows defaults to 200 if you specify SOMAXCONN.

---

### Linux

**epoll Support:** The server uses poll_set internally which leverages epoll on Linux for efficient multi-connection handling.

**File Descriptor Limits:**
```bash
# Check limits
ulimit -n

# Increase for high-concurrency servers
ulimit -n 65536
```

**Thread Limit:** Typically 32,000+ threads on modern Linux, but use thread pools wisely.

---

### macOS

**kqueue Support:** poll_set uses kqueue on macOS/BSD for optimal performance.

**File Descriptor Limits:**
```bash
# Check limits
ulimit -n

# Increase temporarily
ulimit -n 10240
```

---

### BSD

Similar to macOS, uses kqueue for polling. Same file descriptor limit considerations apply.

---

## See Also

**Related Classes:**
- [tcp_server_connection](tcp_server_connection.md) - Base class for connection handlers
- [server_socket](server_socket.md) - TCP server socket for accepting connections
- [tcp_client](tcp_client.md) - TCP client socket for data transfer
- [socket_address](socket_address.md) - Network address representation
- [poll_set](poll_set.md) - I/O multiplexing (used internally)

**Related Documentation:**
- [Exception Handling](exceptions.md) - Network exception hierarchy
- [Examples](examples.md) - Practical usage examples

**Threading & Signals:**
- [Signal](../fb_signal/doc/Signal.md) - Event notification system
- C++ Threading (std::thread, std::mutex, std::condition_variable)

---

**[Back to Index](index.md)** | **[Previous: poll_set](poll_set.md)** | **[Next: udp_server](udp_server.md)**
