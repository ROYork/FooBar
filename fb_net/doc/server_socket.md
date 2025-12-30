# fb::server_socket - TCP Server Socket

## Overview

The [`fb::server_socket`](../include/fb/server_socket.h) class provides a high-level interface for TCP server sockets. It handles binding to addresses, listening for connections, and accepting client connections. The class inherits from `socket_base` and adds server-specific functionality.

**Key Features:**
- Simple bind and listen operations
- Accept client connections as `tcp_client` objects
- Configurable connection backlog
- Address and port reuse options
- Timeout support for accept operations
- Check for pending connections
- Exception-safe RAII design

**Namespace:** `fb`

**Header:** `#include <fb/server_socket.h>`

---

## Quick Start

```cpp
#include <fb/server_socket.h>
#include <fb/tcp_client.h>
#include <iostream>

using namespace fb;

int main()
{
    try {
        // Create and configure server socket
        server_socket server(socket_address::IPv4);
        server.bind(socket_address(8080));  // Bind to port 8080 on all interfaces
        server.listen(10);  // Backlog of 10

        std::cout << "Server listening on port 8080" << std::endl;

        // Accept client connection
        socket_address client_addr;
        tcp_client client = server.accept_connection(client_addr);

        std::cout << "Client connected from: " << client_addr.to_string() << std::endl;

        // Use the client socket
        client.send("Welcome!");

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
```

---

## Construction

### Default Constructor

Creates an unbound IPv4 server socket.

```cpp
server_socket();
```

**Example:**
```cpp
server_socket server;
server.bind(socket_address(8080));
server.listen();
```

---

### Family Constructor

Creates an unbound server socket with specified address family.

```cpp
explicit server_socket(socket_address::Family family);
```

**Parameters:**
- `family` - Address family (IPv4 or IPv6)

**Example:**
```cpp
// IPv4 server
server_socket server_v4(socket_address::IPv4);

// IPv6 server
server_socket server_v6(socket_address::IPv6);
```

---

### Port Constructor

Creates, binds, and listens on the specified port.

```cpp
explicit server_socket(std::uint16_t port, int backlog = 64);
```

**Parameters:**
- `port` - Port number to bind to
- `backlog` - Connection backlog size (default: 64)

**Example:**
```cpp
// Ready to accept connections on port 8080
server_socket server(8080);

// Custom backlog
server_socket server(8080, 128);
```

---

### Address Constructor

Creates, binds, and listens on the specified address.

```cpp
explicit server_socket(const socket_address& address, int backlog = 64);
```

**Parameters:**
- `address` - Address to bind to
- `backlog` - Connection backlog size

**Example:**
```cpp
// Bind to specific interface
server_socket server(socket_address("192.168.1.100", 8080));

// Bind to all interfaces
server_socket server(socket_address("0.0.0.0", 8080));

// IPv6
server_socket server(socket_address(socket_address::IPv6, "::", 8080));
```

---

### Address with Reuse Constructor

Creates with explicit reuse_address control.

```cpp
server_socket(const socket_address& address, int backlog, bool reuse_address);
```

**Parameters:**
- `address` - Address to bind to
- `backlog` - Connection backlog size
- `reuse_address` - Enable SO_REUSEADDR

**Example:**
```cpp
// Disable address reuse (not recommended)
server_socket server(socket_address(8080), 64, false);

// Enable address reuse (recommended, default)
server_socket server(socket_address(8080), 64, true);
```

---

### Move Constructor

```cpp
server_socket(server_socket&& other) noexcept;
```

**Example:**
```cpp
server_socket create_server(uint16_t port)
{
    server_socket server(port);
    return server;  // Move semantics
}

server_socket server = create_server(8080);
```

---

## Server Operations

### bind()

Binds the socket to a local address.

```cpp
void bind(const socket_address& address, bool reuse_address = true);
void bind(const socket_address& address, bool reuse_address, bool reuse_port);
```

**Parameters:**
- `address` - Local address to bind to
- `reuse_address` - Enable SO_REUSEADDR (default: true)
- `reuse_port` - Enable SO_REUSEPORT (default: false)

**Throws:**
- ``std::system_error`` - Bind failed (address in use, permission denied, etc.)

**Example:**
```cpp
server_socket server;

// Bind to all interfaces on port 8080
server.bind(socket_address(8080));

// Bind to specific interface
server.bind(socket_address("192.168.1.100", 8080));

// Bind with address reuse disabled
server.bind(socket_address(8080), false);

// Bind with both address and port reuse
server.bind(socket_address(8080), true, true);
```

---

### listen()

Marks the socket as passive (listening for connections).

```cpp
void listen(int backlog = 64);
```

**Parameters:**
- `backlog` - Maximum length of pending connection queue (default: 64, max: 1024)

**Throws:**
- ``std::system_error`` - Listen failed

**Behavior:**
- Must call `bind()` before `listen()`
- Backlog determines how many pending connections can queue
- Clamped to MAX_BACKLOG (1024)

**Example:**
```cpp
server_socket server;
server.bind(socket_address(8080));
server.listen(10);  // Allow up to 10 pending connections

// Or let system decide
server.listen();  // Default: 64
```

---

### accept_connection()

Accepts a pending client connection.

```cpp
tcp_client accept_connection(socket_address& client_address);
tcp_client accept_connection();
tcp_client accept_connection(socket_address& client_address,
                            const std::chrono::milliseconds& timeout);
tcp_client accept_connection(const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `client_address` - Output parameter for client's address (optional)
- `timeout` - Accept timeout (optional)

**Returns:** Connected `tcp_client` socket

**Throws:**
- ``std::system_error` (std::errc::timed_out)` - Accept timed out (when timeout specified)
- ``std::system_error`` - Accept failed

**Behavior:**
- Blocks until client connects (or timeout)
- Returns new socket for client connection
- Server socket remains in listening state

**Example:**
```cpp
server_socket server(8080);

// Accept without client address
tcp_client client = server.accept_connection();

// Accept with client address
socket_address client_addr;
tcp_client client = server.accept_connection(client_addr);
std::cout << "Client from: " << client_addr.to_string() << std::endl;

// Accept with timeout
try {
    tcp_client client = server.accept_connection(std::chrono::seconds(30));
} catch (const `std::system_error` (std::errc::timed_out)& e) {
    std::cout << "No connection within 30 seconds" << std::endl;
}
```

---

## Configuration

### set_backlog()

Changes the connection backlog size.

```cpp
void set_backlog(int backlog);
```

**Parameters:**
- `backlog` - New backlog size (clamped to MAX_BACKLOG)

**Example:**
```cpp
server_socket server(8080);
server.set_backlog(100);  // Increase backlog
```

---

### set_reuse_address() / get_reuse_address()

Controls SO_REUSEADDR socket option.

```cpp
void set_reuse_address(bool flag);
bool get_reuse_address();
```

**Parameters:**
- `flag` - `true` to enable, `false` to disable

**When to Use:**
- **Enable (recommended)**: Allows immediate rebind after server restart
- **Disable**: Prevents multiple processes binding to same address

**Example:**
```cpp
server_socket server;
server.set_reuse_address(true);  // Allow quick restart
server.bind(socket_address(8080));
```

---

### set_reuse_port() / get_reuse_port()

Controls SO_REUSEPORT socket option (Linux/BSD).

```cpp
void set_reuse_port(bool flag);
bool get_reuse_port();
```

**Parameters:**
- `flag` - `true` to enable, `false` to disable

**When to Use:**
- Load balancing multiple server processes on same port
- Not supported on all platforms

**Example:**
```cpp
server_socket server;
server.set_reuse_port(true);  // Allow multiple servers on same port
server.bind(socket_address(8080));
```

---

## Status Queries

### has_pending_connections()

Checks if connections are waiting to be accepted.

```cpp
bool has_pending_connections(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));
```

**Parameters:**
- `timeout` - Time to wait for pending connection (default: 0 = no wait)

**Returns:** `true` if connection pending, `false` otherwise

**Use Case:**
- Non-blocking accept loops
- Check before calling accept

**Example:**
```cpp
server_socket server(8080);

// Poll for connections
while (running) {
    if (server.has_pending_connections(std::chrono::milliseconds(100))) {
        tcp_client client = server.accept_connection();
        handle_client(std::move(client));
    }
}

// Check without waiting
if (server.has_pending_connections()) {
    // Connection available immediately
}
```

---

### max_connections()

Returns the maximum backlog size.

```cpp
int max_connections() const;
```

**Returns:** MAX_BACKLOG constant (1024)

---

### queue_size()

Returns the current backlog setting.

```cpp
int queue_size() const;
```

**Returns:** Configured backlog size

---

## Complete Examples

### Example 1: Basic Echo Server

```cpp
#include <fb/server_socket.h>
#include <fb/tcp_client.h>
#include <iostream>

void echo_server(uint16_t port)
{
    using namespace fb;

    try {
        // Create and start server
        server_socket server(port);
        std::cout << "Echo server listening on port " << port << std::endl;

        while (true) {
            // Accept client
            socket_address client_addr;
            tcp_client client = server.accept_connection(client_addr);
            std::cout << "Client connected: " << client_addr.to_string() << std::endl;

            // Echo loop
            std::string buffer;
            while (client.receive(buffer, 1024) > 0) {
                std::cout << "Echoing: " << buffer << std::endl;
                client.send(buffer);
            }

            std::cout << "Client disconnected" << std::endl;
        }

    } catch (const std::system_error& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

int main()
{
    fb::initialize_library();
    echo_server(8080);
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Multi-Client Server (Sequential)

```cpp
#include <fb/server_socket.h>
#include <iostream>
#include <vector>

void handle_client(fb::tcp_client client, const fb::socket_address& addr)
{
    std::cout << "Handling client: " << addr.to_string() << std::endl;

    try {
        std::string request;
        client.receive(request, 4096);

        // Process request
        std::string response = process_request(request);

        // Send response
        client.send(response);

    } catch (const fb::std::system_error& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

void multi_client_server(uint16_t port)
{
    using namespace fb;

    server_socket server(port);
    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
        socket_address client_addr;
        tcp_client client = server.accept_connection(client_addr);

        // Handle each client sequentially
        handle_client(std::move(client), client_addr);
    }
}
```

---

### Example 3: Server with Timeout

```cpp
#include <fb/server_socket.h>
#include <iostream>

void server_with_timeout(uint16_t port)
{
    using namespace fb;

    server_socket server(port);
    std::cout << "Server listening with 30s timeout" << std::endl;

    while (true) {
        try {
            // Accept with timeout
            socket_address client_addr;
            tcp_client client = server.accept_connection(
                client_addr,
                std::chrono::seconds(30)
            );

            std::cout << "Client connected: " << client_addr.to_string() << std::endl;
            handle_client(std::move(client));

        } catch (const `std::system_error` (std::errc::timed_out)& e) {
            std::cout << "No connection for 30 seconds, checking status..." << std::endl;
            check_server_status();
        }
    }
}
```

---

### Example 4: Non-Blocking Accept Loop

```cpp
#include <fb/server_socket.h>
#include <iostream>
#include <vector>
#include <thread>

void non_blocking_server(uint16_t port)
{
    using namespace fb;

    server_socket server(port);
    std::cout << "Non-blocking server on port " << port << std::endl;

    std::vector<std::thread> client_threads;
    bool running = true;

    while (running) {
        // Check for pending connections (100ms poll)
        if (server.has_pending_connections(std::chrono::milliseconds(100))) {
            socket_address client_addr;
            tcp_client client = server.accept_connection(client_addr);

            std::cout << "New client: " << client_addr.to_string() << std::endl;

            // Handle in separate thread
            client_threads.emplace_back([](tcp_client c) {
                handle_client(std::move(c));
            }, std::move(client));
        }

        // Do other work here
        perform_maintenance();
    }

    // Wait for all client threads
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
```

---

### Example 5: IPv4 and IPv6 Dual Stack

```cpp
#include <fb/server_socket.h>
#include <thread>
#include <vector>

class DualStackServer
{
public:
    void start(uint16_t port)
    {
        // Create IPv4 server
        server_v4_ = std::make_unique<fb::server_socket>(
            fb::socket_address(fb::socket_address::IPv4, "0.0.0.0", port)
        );

        // Create IPv6 server
        server_v6_ = std::make_unique<fb::server_socket>(
            fb::socket_address(fb::socket_address::IPv6, "::", port)
        );

        // Start accept threads
        thread_v4_ = std::thread(&DualStackServer::accept_loop, this, server_v4_.get(), "IPv4");
        thread_v6_ = std::thread(&DualStackServer::accept_loop, this, server_v6_.get(), "IPv6");

        std::cout << "Dual-stack server listening on port " << port << std::endl;
    }

    void stop()
    {
        running_ = false;
        if (thread_v4_.joinable()) thread_v4_.join();
        if (thread_v6_.joinable()) thread_v6_.join();
    }

private:
    void accept_loop(fb::server_socket* server, const std::string& type)
    {
        while (running_) {
            try {
                if (server->has_pending_connections(std::chrono::milliseconds(100))) {
                    fb::socket_address client_addr;
                    fb::tcp_client client = server->accept_connection(client_addr);

                    std::cout << type << " client: " << client_addr.to_string() << std::endl;

                    handle_client(std::move(client));
                }
            } catch (const fb::std::system_error& e) {
                std::cerr << type << " accept error: " << e.what() << std::endl;
            }
        }
    }

    std::unique_ptr<fb::server_socket> server_v4_;
    std::unique_ptr<fb::server_socket> server_v6_;
    std::thread thread_v4_;
    std::thread thread_v6_;
    std::atomic<bool> running_{true};
};
```

---

## Common Patterns

### Pattern 1: Server Lifecycle Management

```cpp
class Server
{
public:
    void start(uint16_t port)
    {
        server_.bind(fb::socket_address(port));
        server_.listen();
        running_ = true;
        accept_thread_ = std::thread(&Server::accept_loop, this);
    }

    void stop()
    {
        running_ = false;
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
    }

private:
    void accept_loop()
    {
        while (running_) {
            if (server_.has_pending_connections(std::chrono::milliseconds(100))) {
                auto client = server_.accept_connection();
                handle_client(std::move(client));
            }
        }
    }

    fb::server_socket server_;
    std::thread accept_thread_;
    std::atomic<bool> running_{false};
};
```

---

### Pattern 2: Error Recovery

```cpp
void robust_server(uint16_t port)
{
    while (true) {
        try {
            fb::server_socket server(port);
            std::cout << "Server started on port " << port << std::endl;

            while (true) {
                auto client = server.accept_connection();
                handle_client(std::move(client));
            }

        } catch (const fb::`std::system_error`& e) {
            std::cerr << "Server error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Restarting server..." << std::endl;
        }
    }
}
```

---

### Pattern 3: Client Connection Limits

```cpp
void limited_server(uint16_t port, int max_clients)
{
    using namespace fb;

    server_socket server(port);
    server.set_backlog(max_clients);

    std::vector<std::thread> client_threads;

    while (true) {
        if (client_threads.size() < max_clients) {
            auto client = server.accept_connection();

            client_threads.emplace_back([](tcp_client c) {
                handle_client(std::move(c));
            }, std::move(client));

            // Clean up finished threads
            client_threads.erase(
                std::remove_if(client_threads.begin(), client_threads.end(),
                    [](std::thread& t) {
                        if (t.joinable()) return false;
                        return true;
                    }),
                client_threads.end()
            );
        } else {
            // Server full, wait before checking again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
```

---

## Error Handling

### Common Errors

| Error | Cause | Solution |
|-------|-------|----------|
| Address in use | Port already bound | Use different port or set SO_REUSEADDR |
| Permission denied | Port < 1024 without privileges | Use port >= 1024 or run with privileges |
| Too many open files | Exceeded file descriptor limit | Close unused sockets or increase limit |

### Error Handling Example

```cpp
void safe_server_startup(uint16_t port)
{
    using namespace fb;

    try {
        server_socket server;
        server.set_reuse_address(true);
        server.bind(socket_address(port));
        server.listen();

        std::cout << "Server started successfully" << std::endl;

    } catch (const `std::system_error`& e) {
        if (e.code() == EADDRINUSE) {
            std::cerr << "Port " << port << " is already in use" << std::endl;
            std::cerr << "Try a different port or stop the conflicting process" << std::endl;
        } else if (e.code() == EACCES) {
            std::cerr << "Permission denied for port " << port << std::endl;
            std::cerr << "Ports below 1024 require root/admin privileges" << std::endl;
        } else {
            std::cerr << "Failed to start server: " << e.what() << std::endl;
        }
    }
}
```

---

## Performance Considerations

### Backlog Size

```cpp
// Small backlog for low-traffic servers
server.listen(10);

// Large backlog for high-traffic servers
server.listen(128);

// System default
server.listen();  // 64
```

### SO_REUSEADDR

```cpp
// Always enable for production servers
server.set_reuse_address(true);
server.bind(socket_address(port));

// Allows immediate restart without "address in use" errors
```

### Accept Loop Optimization

```cpp
// Efficient: Check for pending connections before blocking
while (running) {
    if (server.has_pending_connections(std::chrono::milliseconds(100))) {
        auto client = server.accept_connection();
        handle_client(std::move(client));
    }
    // Can do other work here
}

// Inefficient: Blocking accept with no timeout
while (running) {
    auto client = server.accept_connection();  // Blocks indefinitely
    handle_client(std::move(client));
}
```

---

## Thread Safety

server_socket is **not thread-safe** for concurrent operations:

```cpp
// BAD: Multiple threads calling accept on same server
fb::server_socket server(8080);
std::thread t1([&]() { auto c = server.accept_connection(); });  // Race!
std::thread t2([&]() { auto c = server.accept_connection(); });  // Race!

// GOOD: Single accept thread, multiple handler threads
fb::server_socket server(8080);
std::thread accept_thread([&]() {
    while (true) {
        auto client = server.accept_connection();
        std::thread handler([c = std::move(client)]() mutable {
            handle_client(std::move(c));
        });
        handler.detach();
    }
});
```

---

## Complete API Reference

| Category | Member | Description |
|----------|--------|-------------|
| **Constructors** | `server_socket()` | Default constructor (IPv4) |
| | `server_socket(Family)` | Specify address family |
| | `server_socket(port)` | Create, bind, and listen |
| | `server_socket(port, backlog)` | With custom backlog |
| | `server_socket(address)` | Bind to specific address |
| | `server_socket(address, backlog)` | With backlog |
| | `server_socket(address, backlog, reuse)` | With reuse control |
| **Server Ops** | `bind(address)` | Bind to local address |
| | `bind(address, reuse_addr)` | With SO_REUSEADDR control |
| | `bind(address, reuse_addr, reuse_port)` | With both reuse options |
| | `listen(backlog)` | Start listening |
| | `accept_connection()` | Accept client (blocking) |
| | `accept_connection(client_addr)` | Accept with address |
| | `accept_connection(timeout)` | Accept with timeout |
| | `accept_connection(client_addr, timeout)` | Both |
| **Configuration** | `set_backlog(int)` | Set connection backlog |
| | `set_reuse_address(bool)` | Control SO_REUSEADDR |
| | `get_reuse_address()` | Get SO_REUSEADDR |
| | `set_reuse_port(bool)` | Control SO_REUSEPORT |
| | `get_reuse_port()` | Get SO_REUSEPORT |
| **Status** | `has_pending_connections(timeout)` | Check for waiting clients |
| | `max_connections()` | Get MAX_BACKLOG |
| | `queue_size()` | Get current backlog |

---

## See Also

- [`tcp_client.md`](tcp_client.md) - TCP client sockets
- [`tcp_server.md`](tcp_server.md) - Multi-threaded server infrastructure
- [`socket_base.md`](socket_base.md) - Base socket functionality
- [`socket_address.md`](socket_address.md) - Network addressing
- [`exceptions.md`](exceptions.md) - Exception handling
- [`examples.md`](examples.md) - Practical usage examples

---

*server_socket - TCP server foundation for modern C++*
