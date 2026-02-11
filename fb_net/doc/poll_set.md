# fb::poll_set - Socket Polling and Multiplexing

## Overview

The [`fb::poll_set`](../include/fb/poll_set.h) class provides efficient I/O multiplexing for monitoring multiple sockets simultaneously. It abstracts platform-specific polling mechanisms (epoll on Linux, kqueue on BSD/macOS, select as fallback) into a unified, easy-to-use interface.

**Key Features:**
- Monitor multiple sockets for I/O readiness
- Platform-optimized (epoll/kqueue/select)
- Handle 1000+ sockets efficiently
- Read, write, and error event monitoring
- Timeout support
- Zero-copy socket management
- Thread-safe operations

**Namespace:** `fb`

**Header:** `#include <fb/poll_set.h>`

---

## Quick Start

```cpp
#include <fb/poll_set.h>
#include <fb/tcp_client.h>
#include <iostream>

using namespace fb;

int main()
{
    try {
        // Create poll set
        poll_set poller;

        // Create and add sockets
        tcp_client socket1, socket2, socket3;
        socket1.connect(socket_address("server1.com", 80));
        socket2.connect(socket_address("server2.com", 80));
        socket3.connect(socket_address("server3.com", 80));

        // Add sockets for monitoring
        poller.add(socket1, poll_set::POLL_READ | poll_set::POLL_WRITE);
        poller.add(socket2, poll_set::POLL_READ);
        poller.add(socket3, poll_set::POLL_READ);

        // Wait for events (1 second timeout)
        int ready = poller.poll(std::chrono::seconds(1));

        std::cout << ready << " sockets ready" << std::endl;

        // Process events
        for (const auto& event : poller.events()) {
            if (event.mode & poll_set::POLL_READ) {
                // Socket ready for reading
                std::string data;
                static_cast<tcp_client*>(event.socket_ptr)->receive(data, 4096);
            }

            if (event.mode & poll_set::POLL_WRITE) {
                // Socket ready for writing
                static_cast<tcp_client*>(event.socket_ptr)->send("Data");
            }
        }

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
```

---

## Polling Modes

### Mode Enumeration

```cpp
enum Mode : int
{
    POLL_READ = 1,      // Monitor for read availability
    POLL_WRITE = 2,     // Monitor for write availability
    POLL_ERROR = 4      // Monitor for error conditions
};
```

**Usage:**
- **POLL_READ**: Socket has data to read or connection accepted
- **POLL_WRITE**: Socket ready for writing (send won't block)
- **POLL_ERROR**: Error condition on socket
- **Combined**: Use bitwise OR for multiple modes

**Example:**
```cpp
// Monitor for both read and write
poller.add(socket, poll_set::POLL_READ | poll_set::POLL_WRITE);

// Monitor for read only
poller.add(socket, poll_set::POLL_READ);

// Monitor for all events
poller.add(socket, poll_set::POLL_READ | poll_set::POLL_WRITE | poll_set::POLL_ERROR);
```

---

## Construction

### Default Constructor

Creates an empty poll set.

```cpp
poll_set();
```

**Example:**
```cpp
poll_set poller;
```

---

### Move Constructor

```cpp
poll_set(poll_set&& other) noexcept;
```

**Note:** poll_set is move-only (no copy constructor)

---

## Socket Management

### add()

Adds a socket to the poll set.

```cpp
void add(const socket_base& socket, int mode);
```

**Parameters:**
- `socket` - Socket to monitor
- `mode` - Events to monitor (POLL_READ, POLL_WRITE, POLL_ERROR or combination)

**Throws:** ``std::system_error`` if socket already in set

**Example:**
```cpp
poll_set poller;
tcp_client socket;
socket.connect(socket_address("example.com", 80));

// Add for reading
poller.add(socket, poll_set::POLL_READ);

// Add for reading and writing
poller.add(socket, poll_set::POLL_READ | poll_set::POLL_WRITE);
```

---

### remove()

Removes a socket from the poll set.

```cpp
void remove(const socket_base& socket);
```

**Parameters:**
- `socket` - Socket to remove

**Example:**
```cpp
poller.remove(socket);
```

---

### update()

Updates the events monitored for a socket.

```cpp
void update(const socket_base& socket, int mode);
```

**Parameters:**
- `socket` - Socket to update
- `mode` - New event mode

**Use Case:**
- Change from read-only to read-write
- Disable write monitoring after send completes

**Example:**
```cpp
// Initially monitor for writing only
poller.add(socket, poll_set::POLL_WRITE);

// After data sent, switch to reading
poller.update(socket, poll_set::POLL_READ);
```

---

### has()

Checks if a socket is in the poll set.

```cpp
bool has(const socket_base& socket) const;
```

**Returns:** `true` if socket is monitored, `false` otherwise

**Example:**
```cpp
if (poller.has(socket)) {
    poller.remove(socket);
}
```

---

### get_mode()

Gets the current monitoring mode for a socket.

```cpp
int get_mode(const socket_base& socket) const;
```

**Returns:** Current mode flags

**Example:**
```cpp
int mode = poller.get_mode(socket);
if (mode & poll_set::POLL_READ) {
    std::cout << "Monitoring for read" << std::endl;
}
```

---

### clear()

Removes all sockets from the poll set.

```cpp
void clear();
```

**Example:**
```cpp
poller.clear();  // Remove all sockets
```

---

### empty()

Checks if the poll set is empty.

```cpp
bool empty() const;
```

**Returns:** `true` if no sockets, `false` otherwise

---

### count()

Returns the number of sockets being monitored.

```cpp
std::size_t count() const;
```

**Returns:** Number of sockets in the set

**Example:**
```cpp
std::cout << "Monitoring " << poller.count() << " sockets" << std::endl;
```

---

## Polling Operations

### poll()

Waits for events on monitored sockets.

```cpp
int poll(const std::chrono::milliseconds& timeout);
int poll(std::vector<SocketEvent>& events, const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `timeout` - Maximum time to wait (0 = return immediately, negative = wait indefinitely)
- `events` - Output vector for events (optional)

**Returns:** Number of sockets with events

**Behavior:**
- Blocks until events occur or timeout
- Results accessible via `events()` method
- Returns 0 on timeout

**Example:**
```cpp
// Poll with 1 second timeout
int ready = poller.poll(std::chrono::seconds(1));

if (ready > 0) {
    std::cout << ready << " sockets have events" << std::endl;

    for (const auto& event : poller.events()) {
        // Process event
    }
} else {
    std::cout << "Timeout - no events" << std::endl;
}

// Poll with output vector
std::vector<SocketEvent> events;
int ready = poller.poll(events, std::chrono::milliseconds(500));
```

---

### events()

Gets the events from the last poll operation.

```cpp
const std::vector<SocketEvent>& events() const;
```

**Returns:** Vector of socket events

**Example:**
```cpp
poller.poll(std::chrono::seconds(1));

for (const auto& event : poller.events()) {
    auto* socket = static_cast<tcp_client*>(event.socket_ptr);

    if (event.mode & poll_set::POLL_READ) {
        // Handle read
    }
    if (event.mode & poll_set::POLL_WRITE) {
        // Handle write
    }
    if (event.mode & poll_set::POLL_ERROR) {
        // Handle error
    }
}
```

---

### clear_events()

Clears the event list from the last poll.

```cpp
void clear_events();
```

---

## Platform Information

### polling_method()

Returns the polling mechanism being used.

```cpp
static const char* polling_method();
```

**Returns:** "epoll" (Linux), "kqueue" (BSD/macOS), or "select" (fallback)

**Example:**
```cpp
std::cout << "Using: " << poll_set::polling_method() << std::endl;
```

---

### efficient_polling()

Checks if an efficient polling mechanism is available.

```cpp
static bool efficient_polling();
```

**Returns:**
- `true` for epoll or kqueue
- `false` for select

**Example:**
```cpp
if (poll_set::efficient_polling()) {
    std::cout << "Efficient polling available" << std::endl;
} else {
    std::cout << "Using select (less efficient)" << std::endl;
}
```

---

## SocketEvent Structure

```cpp
struct SocketEvent
{
    socket_base* socket_ptr;  // Pointer to socket with events
    int mode;                  // Event flags (POLL_READ, POLL_WRITE, POLL_ERROR)
};
```

**Usage:**
```cpp
for (const auto& event : poller.events()) {
    auto* socket = static_cast<tcp_client*>(event.socket_ptr);

    if (event.mode & poll_set::POLL_READ) {
        socket->receive(buffer, size);
    }
}
```

---

## Complete Examples

### Example 1: Multi-Client Manager

```cpp
#include <fb/poll_set.h>
#include <fb/tcp_client.h>
#include <map>
#include <iostream>

class ClientManager
{
public:
    void add_client(const std::string& name, const socket_address& addr)
    {
        auto socket = std::make_unique<tcp_client>();
        socket->connect(addr);

        poller_.add(*socket, poll_set::POLL_READ);
        clients_[socket.get()] = {name, std::move(socket)};
    }

    void process_events()
    {
        int ready = poller_.poll(std::chrono::milliseconds(100));

        if (ready > 0) {
            for (const auto& event : poller_.events()) {
                auto* socket = static_cast<tcp_client*>(event.socket_ptr);
                auto& client = clients_[socket];

                if (event.mode & poll_set::POLL_READ) {
                    std::string data;
                    int bytes = socket->receive(data, 4096);

                    if (bytes > 0) {
                        std::cout << client.name << ": " << data << std::endl;
                    } else {
                        // Connection closed
                        std::cout << client.name << " disconnected" << std::endl;
                        poller_.remove(*socket);
                        clients_.erase(socket);
                    }
                }
            }
        }
    }

private:
    struct ClientInfo {
        std::string name;
        std::unique_ptr<tcp_client> socket;
    };

    poll_set poller_;
    std::map<tcp_client*, ClientInfo> clients_;
};

int main()
{
    fb::initialize_library();

    ClientManager manager;
    manager.add_client("Server1", fb::socket_address("192.168.1.1", 80));
    manager.add_client("Server2", fb::socket_address("192.168.1.2", 80));
    manager.add_client("Server3", fb::socket_address("192.168.1.3", 80));

    while (true) {
        manager.process_events();
    }

    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Server with Multiple Connections

```cpp
#include <fb/poll_set.h>
#include <fb/server_socket.h>
#include <fb/tcp_client.h>
#include <vector>
#include <iostream>

void multi_connection_server(uint16_t port)
{
    using namespace fb;

    // Create server socket
    server_socket server(port);
    std::cout << "Server listening on port " << port << std::endl;

    poll_set poller;
    std::vector<std::unique_ptr<tcp_client>> clients;

    // Add server socket for accepting
    poller.add(server, poll_set::POLL_READ);

    while (true) {
        int ready = poller.poll(std::chrono::milliseconds(100));

        if (ready > 0) {
            for (const auto& event : poller.events()) {
                if (event.socket_ptr == &server) {
                    // New client connection
                    socket_address client_addr;
                    auto client = std::make_unique<tcp_client>(
                        server.accept_connection(client_addr)
                    );

                    std::cout << "Client connected: " << client_addr.to_string() << std::endl;

                    poller.add(*client, poll_set::POLL_READ);
                    clients.push_back(std::move(client));

                } else {
                    // Existing client has data
                    auto* client = static_cast<tcp_client*>(event.socket_ptr);

                    if (event.mode & poll_set::POLL_READ) {
                        std::string data;
                        int bytes = client->receive(data, 1024);

                        if (bytes > 0) {
                            std::cout << "Received: " << data << std::endl;
                            client->send(data);  // Echo
                        } else {
                            // Client disconnected
                            std::cout << "Client disconnected" << std::endl;
                            poller.remove(*client);

                            // Remove from vector
                            clients.erase(
                                std::remove_if(clients.begin(), clients.end(),
                                    [client](const auto& c) { return c.get() == client; }),
                                clients.end()
                            );
                        }
                    }
                }
            }
        }
    }
}
```

---

### Example 3: Timeout-Based Monitoring

```cpp
#include <fb/poll_set.h>
#include <fb/tcp_client.h>
#include <chrono>
#include <iostream>

void monitor_with_timeout()
{
    using namespace fb;

    poll_set poller;
    std::vector<std::unique_ptr<tcp_client>> sockets;

    // Create monitoring sockets
    for (int i = 0; i < 5; ++i) {
        auto socket = std::make_unique<tcp_client>();
        socket->connect(socket_address("server" + std::to_string(i) + ".com", 80));
        poller.add(*socket, poll_set::POLL_READ);
        sockets.push_back(std::move(socket));
    }

    auto last_check = std::chrono::steady_clock::now();

    while (true) {
        // Poll with short timeout
        int ready = poller.poll(std::chrono::milliseconds(100));

        if (ready > 0) {
            for (const auto& event : poller.events()) {
                auto* socket = static_cast<tcp_client*>(event.socket_ptr);

                std::string data;
                socket->receive(data, 4096);
                process_data(data);
            }
        }

        // Periodic maintenance
        auto now = std::chrono::steady_clock::now();
        if (now - last_check > std::chrono::seconds(10)) {
            perform_periodic_maintenance();
            last_check = now;
        }
    }
}
```

---

### Example 4: UDP Polling

```cpp
#include <fb/poll_set.h>
#include <fb/udp_socket.h>
#include <iostream>

void udp_polling_example()
{
    using namespace fb;

    poll_set poller;

    // Create multiple UDP sockets
    udp_socket socket1(socket_address(5000));
    udp_socket socket2(socket_address(5001));
    udp_socket socket3(socket_address(5002));

    poller.add(socket1, poll_set::POLL_READ);
    poller.add(socket2, poll_set::POLL_READ);
    poller.add(socket3, poll_set::POLL_READ);

    std::cout << "Monitoring " << poller.count() << " UDP sockets" << std::endl;

    while (true) {
        int ready = poller.poll(std::chrono::seconds(1));

        if (ready > 0) {
            for (const auto& event : poller.events()) {
                auto* socket = static_cast<udp_socket*>(event.socket_ptr);

                char buffer[1024];
                socket_address sender;
                int bytes = socket->receive_from(buffer, sizeof(buffer), sender);

                std::cout << "Received " << bytes << " bytes from "
                         << sender.to_string() << std::endl;
            }
        }
    }
}
```

---

### Example 5: Load Balancer

```cpp
#include <fb/poll_set.h>
#include <fb/tcp_client.h>
#include <vector>
#include <algorithm>

class LoadBalancer
{
public:
    void add_backend(const socket_address& addr)
    {
        auto socket = std::make_unique<tcp_client>();
        socket->connect(addr);

        Backend backend;
        backend.socket = std::move(socket);
        backend.connections = 0;

        backends_.push_back(std::move(backend));
    }

    tcp_client* get_least_loaded()
    {
        auto it = std::min_element(backends_.begin(), backends_.end(),
            [](const Backend& a, const Backend& b) {
                return a.connections < b.connections;
            });

        if (it != backends_.end()) {
            it->connections++;
            return it->socket.get();
        }

        return nullptr;
    }

    void process_responses()
    {
        poll_set poller;

        for (auto& backend : backends_) {
            poller.add(*backend.socket, poll_set::POLL_READ);
        }

        int ready = poller.poll(std::chrono::milliseconds(100));

        for (const auto& event : poller.events()) {
            auto* socket = static_cast<tcp_client*>(event.socket_ptr);

            std::string response;
            socket->receive(response, 4096);

            // Forward response to client
            forward_to_client(response);

            // Decrease connection count
            for (auto& backend : backends_) {
                if (backend.socket.get() == socket) {
                    backend.connections--;
                    break;
                }
            }
        }
    }

private:
    struct Backend {
        std::unique_ptr<tcp_client> socket;
        int connections;
    };

    std::vector<Backend> backends_;
};
```

---

## Common Patterns

### Pattern 1: Edge-Triggered Processing

```cpp
void edge_triggered_loop(poll_set& poller)
{
    while (true) {
        int ready = poller.poll(std::chrono::milliseconds(100));

        if (ready > 0) {
            for (const auto& event : poller.events()) {
                // Process all available data
                auto* socket = static_cast<tcp_client*>(event.socket_ptr);

                while (socket->poll_read(std::chrono::milliseconds(0))) {
                    std::string data;
                    if (socket->receive(data, 4096) <= 0) {
                        break;
                    }
                    process_data(data);
                }
            }
        }
    }
}
```

---

### Pattern 2: Dynamic Socket Management

```cpp
void dynamic_socket_management(poll_set& poller,
                              std::vector<std::unique_ptr<tcp_client>>& sockets)
{
    while (true) {
        int ready = poller.poll(std::chrono::milliseconds(100));

        for (const auto& event : poller.events()) {
            auto* socket = static_cast<tcp_client*>(event.socket_ptr);

            std::string data;
            if (socket->receive(data, 4096) <= 0) {
                // Remove dead socket
                poller.remove(*socket);
                sockets.erase(
                    std::remove_if(sockets.begin(), sockets.end(),
                        [socket](const auto& s) { return s.get() == socket; }),
                    sockets.end()
                );
            }
        }

        // Add new sockets as needed
        if (should_add_socket()) {
            auto new_socket = create_socket();
            poller.add(*new_socket, poll_set::POLL_READ);
            sockets.push_back(std::move(new_socket));
        }
    }
}
```

---

## Performance Considerations

### Polling Method

```cpp
// Check which method is used
std::cout << "Polling method: " << poll_set::polling_method() << std::endl;

// Performance characteristics:
// - epoll (Linux): O(1) for add/remove/poll, handles 10,000+ sockets
// - kqueue (BSD/macOS): O(1) for add/remove/poll, handles 10,000+ sockets
// - select (fallback): O(n) for poll, limited to ~1024 sockets
```

### Socket Count

```cpp
// Good: Efficient for any socket count with epoll/kqueue
poll_set poller;
for (int i = 0; i < 10000; ++i) {
    poller.add(sockets[i], poll_set::POLL_READ);
}

// Less efficient with select (>1024 sockets)
```

### Timeout Selection

```cpp
// Tight loop: Short timeout for responsiveness
poller.poll(std::chrono::milliseconds(10));

// Balanced: Medium timeout
poller.poll(std::chrono::milliseconds(100));

// Low CPU: Longer timeout
poller.poll(std::chrono::seconds(1));

// Blocking: Wait indefinitely
poller.poll(std::chrono::milliseconds(-1));
```

---

## Error Handling

### Socket Removal on Error

```cpp
poller.poll(std::chrono::seconds(1));

for (const auto& event : poller.events()) {
    if (event.mode & poll_set::POLL_ERROR) {
        std::cerr << "Socket error detected" << std::endl;

        auto* socket = event.socket_ptr;
        poller.remove(*socket);

        // Handle error and cleanup socket
    }
}
```

---

## Thread Safety

poll_set is **not thread-safe** for concurrent operations:

```cpp
// BAD: Multiple threads modifying same poll_set
poll_set poller;
std::thread t1([&]() { poller.add(socket1, poll_set::POLL_READ); });  // Race!
std::thread t2([&]() { poller.poll(std::chrono::seconds(1)); });     // Race!

// GOOD: Each thread has its own poll_set
std::thread t1([]() {
    poll_set poller1;
    // Use poller1
});
```

---

## Complete API Reference

| Category | Member | Description |
|----------|--------|-------------|
| **Construction** | `poll_set()` | Default constructor |
| | `poll_set(poll_set&&)` | Move constructor |
| **Socket Management** | `add(socket, mode)` | Add socket to poll set |
| | `remove(socket)` | Remove socket from set |
| | `update(socket, mode)` | Update monitoring mode |
| | `has(socket)` | Check if socket in set |
| | `get_mode(socket)` | Get current mode |
| | `clear()` | Remove all sockets |
| | `empty()` | Check if empty |
| | `count()` | Get socket count |
| **Polling** | `poll(timeout)` | Wait for events |
| | `poll(events, timeout)` | Wait with output vector |
| | `events()` | Get last poll results |
| | `clear_events()` | Clear event list |
| **Platform Info** | `polling_method()` | Get polling mechanism |
| | `efficient_polling()` | Check if efficient |

---

## See Also

- [`tcp_client.md`](tcp_client.md) - TCP client sockets
- [`udp_socket.md`](udp_socket.md) - UDP sockets
- [`server_socket.md`](server_socket.md) - Server sockets
- [`examples.md`](examples.md) - Practical usage examples

---

*poll_set - Efficient I/O multiplexing for modern C++*
