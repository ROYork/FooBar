# udp_server - Multi-threaded UDP Server

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Construction](#construction)
- [Server Lifecycle Management](#server-lifecycle-management)
- [Server Configuration](#server-configuration)
- [Server Status and Statistics](#server-status-and-statistics)
- [Signal Events](#signal-events)
- [Handler Factory Pattern](#handler-factory-pattern)
- [Threading Model](#threading-model)
- [Packet Data Structure](#packet-data-structure)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)
- [Error Handling](#error-handling)
- [Performance Considerations](#performance-considerations)
- [Platform-Specific Notes](#platform-specific-notes)
- [See Also](#see-also)

---

## Overview

The `udp_server` class provides a robust, multi-threaded UDP server implementation that receives datagrams and dispatches them to worker threads for processing. It manages a thread pool, packet queue, and provides comprehensive lifecycle management with graceful shutdown support.

**Key Features:**
- **Multi-threaded Architecture** - Separate receiver thread and configurable worker thread pool
- **Dynamic Thread Pooling** - Automatically scales worker threads based on load
- **Packet Queue Management** - Configurable queue size with overflow handling
- **Dual Handler Modes** - Factory-based (one handler per packet) or shared handler (reused)
- **Signal-based Events** - Real-time notifications for server and packet events
- **Graceful Shutdown** - Configurable timeout for clean termination
- **Statistics & Monitoring** - Runtime metrics for packets, drops, and throughput
- **Exception Safety** - Robust error handling with virtual hooks for custom handling

**Architecture:**
```
                                    udp_server
                                        |
                    +-------------------+-------------------+
                    |                                       |
              Receiver Thread                     Worker Thread Pool
                    |                                       |
           (receives datagrams)                   (processes packets)
                    |                                       |
                    v                                       v
             Packet Queue <---------------------------> Handler Instances
                (FIFO)                             (factory or shared)
```

**Include:** `<fb/udp_server.h>`
**Namespace:** `fb`
**Platform Support:** Windows, macOS, Linux, BSD

---

## Quick Start

### Basic UDP Server Example

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

// Define your packet handler
class EchoHandler : public udp_handler {
public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string message(static_cast<const char*>(buffer), length);
        std::cout << "Received from " << sender_address.to_string()
                  << ": " << message << std::endl;

        // In a real server, you'd send a response here
        // (requires access to server socket, typically via factory context)
    }
};

int main() {
    fb::initialize_library();

    // Create and bind UDP socket
    udp_socket server_sock(socket_address::IPv4);
    server_sock.bind(socket_address("0.0.0.0", 8888));

    // Create shared handler
    auto handler = std::make_shared<EchoHandler>();

    // Create server with shared handler
    udp_server server(std::move(server_sock), handler);
    server.start();

    std::cout << "UDP server listening on port 8888\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

## Construction

### Default Constructor

```cpp
udp_server();
```

Constructs an idle server with default configuration. The server is in an unconfigured state and requires calling `set_server_socket()` and either `set_handler_factory()` or `set_shared_handler()` before starting.

**Default Configuration:**
- `max_threads` = 10
- `max_queued` = 1000 packets
- `packet_buffer_size` = 65507 bytes (max UDP payload)
- `packet_timeout` = 0 ms (no timeout)

**Example:**
```cpp
udp_server server;
server.set_server_socket(std::move(my_socket));
server.set_shared_handler(my_handler);
server.start();
```

---

### Factory-Based Constructor

```cpp
udp_server(udp_socket server_socket,
          HandlerFactory handler_factory,
          std::size_t max_threads = 0,
          std::size_t max_queued = 0);
```

Constructs a server that creates a new handler instance for each packet using the provided factory.

**Parameters:**
- `server_socket` - Bound UDP socket for receiving datagrams
- `handler_factory` - Factory function to create udp_handler instances
- `max_threads` - Maximum number of worker threads (0 = use default 10)
- `max_queued` - Maximum queued packets (0 = use default 1000)

**Example:**
```cpp
udp_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8888));

auto factory = [](const udp_server::PacketData& packet) {
    return std::make_unique<MyHandler>(packet.sender_address);
};

udp_server server(std::move(sock), factory, 20, 2000);
```

---

### Shared Handler Constructor

```cpp
udp_server(udp_socket server_socket,
          std::shared_ptr<udp_handler> handler,
          std::size_t max_threads = 0,
          std::size_t max_queued = 0);
```

Constructs a server that reuses a single handler instance for all packets (must be thread-safe).

**Parameters:**
- `server_socket` - Bound UDP socket for receiving datagrams
- `handler` - Shared handler instance for all packets
- `max_threads` - Maximum number of worker threads (0 = use default 10)
- `max_queued` - Maximum queued packets (0 = use default 1000)

**Example:**
```cpp
udp_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8888));

auto handler = std::make_shared<MyThreadSafeHandler>();

udp_server server(std::move(sock), handler);
```

---

### Move Semantics

```cpp
udp_server(udp_server&& other) noexcept;
udp_server& operator=(udp_server&& other) noexcept;
```

The server is **move-only** (no copy construction or assignment). Move operations transfer configuration and state.

**Behavior:**
- Moved-from server becomes unconfigured
- Active servers cannot be moved (stop first)
- Worker threads are not transferred (cannot move threads)

**Example:**
```cpp
udp_server create_server() {
    udp_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 8888));

    auto handler = std::make_shared<MyHandler>();
    return udp_server(std::move(sock), handler);  // Move construction
}

udp_server server = create_server();  // Moved into server
```

---

### Handler Factory Type

```cpp
using HandlerFactory = std::function<
    std::unique_ptr<udp_handler>(const PacketData& packet_data)
>;
```

Factory function signature that creates handler instances for each packet.

**Parameters:**
- `const PacketData&` - Packet information (buffer, sender, timestamp)

**Returns:** `std::unique_ptr<udp_handler>` (or derived class)

**Factory can return `nullptr`** to drop a packet without processing.

---

## Server Lifecycle Management

### start()

```cpp
void start();
```

Starts receiving datagrams and dispatching them to the worker thread pool.

**Behavior:**
1. Validates configuration (socket and handler must be set)
2. Calls virtual `on_server_starting()` hook
3. Starts receiver thread
4. Starts initial worker threads
5. Emits `onServerStarted` signal
6. Sets `is_running()` to `true`

**Throws:**
- `std::runtime_error` - If already running or configuration incomplete
- `std::system_error` - If UDP socket is invalid or closed

**Example:**
```cpp
udp_server server(std::move(sock), handler);
server.start();  // Server is now receiving packets
std::cout << "Server running with " << server.thread_count() << " threads\n";
```

---

### stop()

```cpp
void stop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(5000));
```

Stops receiving packets and shuts down worker threads gracefully.

**Parameters:**
- `timeout` - Maximum time to wait for threads to finish (default: 5000ms)

**Behavior:**
1. Sets internal stop flags
2. Emits `onServerStopping` signal
3. Calls virtual `on_server_stopping()` hook
4. Closes server socket to stop receiving
5. Wakes up all worker threads
6. Joins receiver thread
7. Joins worker threads (detaches if timeout exceeded)
8. Emits `onServerStopped` signal

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

**Returns:** `true` if receiver thread is running and server has not been stopped

**Example:**
```cpp
if (server.is_running()) {
    std::cout << "Server active: " << server.total_packets()
              << " packets received\n";
}
```

---

## Server Configuration

All configuration methods throw `std::runtime_error` if called while the server is running. Stop the server first to reconfigure.

### set_server_socket()

```cpp
void set_server_socket(udp_socket server_socket);
```

Provides a UDP socket to the server. Socket must already be bound to an address.

**Example:**
```cpp
udp_socket sock(socket_address::IPv4);
sock.bind(socket_address("0.0.0.0", 8888));

udp_server server;
server.set_server_socket(std::move(sock));
```

---

### set_handler_factory()

```cpp
void set_handler_factory(HandlerFactory factory);
```

Installs a factory that creates a new handler for each packet.

**Example:**
```cpp
auto factory = [](const udp_server::PacketData& packet) {
    std::cout << "Creating handler for packet from "
              << packet.sender_address.to_string() << "\n";
    return std::make_unique<MyHandler>(packet);
};

server.set_handler_factory(factory);
```

---

### set_shared_handler()

```cpp
void set_shared_handler(std::shared_ptr<udp_handler> handler);
```

Installs a shared handler that processes all packets (must be thread-safe).

**Example:**
```cpp
auto handler = std::make_shared<ThreadSafeHandler>();
server.set_shared_handler(handler);
```

---

### set_max_threads()

```cpp
void set_max_threads(std::size_t max_threads);
```

Configures the upper bound on worker threads. Server dynamically scales based on load.

**Parameters:**
- `max_threads` - Maximum worker count (0 = use default 10)

**Example:**
```cpp
server.set_max_threads(20);  // Allow up to 20 concurrent packet handlers
```

---

### set_max_queued()

```cpp
void set_max_queued(std::size_t max_queued);
```

Sets the maximum number of pending packets waiting for a worker thread.

**Parameters:**
- `max_queued` - Queue capacity in packets (0 = use default 1000)

**Behavior:**
- When queue is full, new packets are **dropped** (not queued)
- Prevents unbounded memory growth under heavy load
- Dropped packets increment `dropped_packets()` counter

**Example:**
```cpp
server.set_max_queued(5000);  // Allow 5000 pending packets
```

---

### set_packet_buffer_size()

```cpp
void set_packet_buffer_size(std::size_t size);
```

Configures the receive buffer size for each packet.

**Parameters:**
- `size` - Buffer size in bytes (default: 65507 - max UDP payload)

**Example:**
```cpp
server.set_packet_buffer_size(1500);  // Standard MTU size
```

---

### set_packet_timeout()

```cpp
void set_packet_timeout(const std::chrono::milliseconds& timeout);
```

Configures timeout for packet reception (currently used for receive operations).

**Parameters:**
- `timeout` - Reception timeout (0 = no timeout)

**Example:**
```cpp
server.set_packet_timeout(std::chrono::seconds(1));
```

---

## Server Status and Statistics

### server_socket()

```cpp
const udp_socket& server_socket() const;
```

Accesses the UDP socket.

**Returns:** Reference to the configured udp_socket

**Throws:** `std::runtime_error` if no socket has been configured

**Example:**
```cpp
std::cout << "Server bound to: " << server.server_socket().address().to_string() << "\n";
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

### total_packets()

```cpp
std::uint64_t total_packets() const;
```

Retrieves the total number of packets received since server start.

**Returns:** Total packets received (includes processed and dropped)

**Thread-Safe:** Yes (atomic)

**Example:**
```cpp
std::cout << "Total packets: " << server.total_packets() << "\n";
```

---

### processed_packets()

```cpp
std::uint64_t processed_packets() const;
```

Gets the number of packets successfully processed by handlers.

**Returns:** Processed packet count

**Thread-Safe:** Yes (atomic)

**Example:**
```cpp
std::cout << "Processed: " << server.processed_packets() << "\n";
```

---

### dropped_packets()

```cpp
std::uint64_t dropped_packets() const;
```

Gets the number of packets dropped due to queue overflow.

**Returns:** Dropped packet count

**Thread-Safe:** Yes (atomic)

**Example:**
```cpp
if (server.dropped_packets() > 0) {
    std::cout << "Warning: " << server.dropped_packets()
              << " packets dropped!\n";
}
```

---

### queued_packets()

```cpp
std::size_t queued_packets() const;
```

Reports how many packets are waiting for a worker thread.

**Returns:** Pending packet count in the queue

**Thread-Safe:** Yes

**Example:**
```cpp
if (server.queued_packets() > 500) {
    std::cout << "Warning: High packet queue depth!\n";
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

The server emits signals for monitoring and observability. **All signals are emitted on receiver/worker threads**. Ensure thread-safe handling.

### Server Lifecycle Signals

#### onServerStarted

```cpp
fb::signal<> onServerStarted;
```

Emitted when the server successfully starts.

**Example:**
```cpp
server.onServerStarted.connect([]() {
    std::cout << "[EVENT] UDP Server started\n";
});
```

---

#### onServerStopping

```cpp
fb::signal<> onServerStopping;
```

Emitted when the server begins shutdown.

**Example:**
```cpp
server.onServerStopping.connect([]() {
    std::cout << "[EVENT] Server stopping...\n";
});
```

---

#### onServerStopped

```cpp
fb::signal<> onServerStopped;
```

Emitted when the server has completely stopped.

**Example:**
```cpp
server.onServerStopped.connect([]() {
    std::cout << "[EVENT] Server stopped\n";
});
```

---

### Packet Reception Signals

#### onPacketReceived

```cpp
fb::signal<const void*, std::size_t, const socket_address&> onPacketReceived;
```

Emitted when a packet is received (before queuing).

**Parameters:**
- `const void*` - Packet buffer
- `std::size_t` - Packet length
- `const socket_address&` - Sender address

**Example:**
```cpp
server.onPacketReceived.connect([](const void* data, std::size_t len, const socket_address& sender) {
    std::cout << "[RX] " << len << " bytes from " << sender.to_string() << "\n";
});
```

---

#### onTotalPacketsChanged

```cpp
fb::signal<std::size_t> onTotalPacketsChanged;
```

Emitted when total packet count changes.

**Example:**
```cpp
server.onTotalPacketsChanged.connect([](size_t total) {
    if (total % 1000 == 0) {
        std::cout << "[STATS] Total packets: " << total << "\n";
    }
});
```

---

#### onProcessedPacketsChanged

```cpp
fb::signal<std::size_t> onProcessedPacketsChanged;
```

Emitted when processed packet count changes.

**Example:**
```cpp
server.onProcessedPacketsChanged.connect([](size_t processed) {
    std::cout << "[STATS] Processed: " << processed << "\n";
});
```

---

#### onDroppedPacketsChanged

```cpp
fb::signal<std::size_t> onDroppedPacketsChanged;
```

Emitted when a packet is dropped due to queue overflow.

**Example:**
```cpp
server.onDroppedPacketsChanged.connect([](size_t dropped) {
    std::cerr << "[WARNING] Packets dropped: " << dropped << "\n";
});
```

---

#### onQueuedPacketsChanged

```cpp
fb::signal<std::size_t> onQueuedPacketsChanged;
```

Emitted when queued packet count changes.

**Example:**
```cpp
server.onQueuedPacketsChanged.connect([](size_t queued) {
    if (queued > 100) {
        std::cout << "[WARNING] High queue: " << queued << " packets\n";
    }
});
```

---

### Threading Signals

#### onWorkerThreadCreated

```cpp
fb::signal<std::size_t> onWorkerThreadCreated;
```

Emitted when a new worker thread is created.

**Parameters:**
- `std::size_t` - New thread count

**Example:**
```cpp
server.onWorkerThreadCreated.connect([](size_t count) {
    std::cout << "[THREAD] Worker created, total: " << count << "\n";
});
```

---

#### onActiveThreadsChanged

```cpp
fb::signal<std::size_t> onActiveThreadsChanged;
```

Emitted when active thread count changes.

**Example:**
```cpp
server.onActiveThreadsChanged.connect([](size_t active) {
    std::cout << "[THREAD] Active threads: " << active << "\n";
});
```

---

### Error Signal

#### onException

```cpp
fb::signal<const std::exception&, const std::string&> onException;
```

Emitted when an exception occurs during server operation.

**Parameters:**
- `const std::exception&` - The exception that was thrown
- `const std::string&` - Context string (e.g., "receiver_thread", "worker_thread")

**Example:**
```cpp
server.onException.connect([](const std::exception& ex, const std::string& ctx) {
    std::cerr << "[ERROR] Exception in " << ctx << ": " << ex.what() << "\n";
});
```

---

## Handler Factory Pattern

The handler factory determines how packets are processed.

### Factory Patterns

#### 1. Simple Factory (One Handler Per Packet)

```cpp
auto factory = [](const udp_server::PacketData& packet) {
    return std::make_unique<MyHandler>(packet.sender_address);
};
```

**Use when:**
- Each packet needs isolated processing
- Handlers maintain per-packet state
- No shared state required

---

#### 2. Shared Handler (Thread-Safe)

```cpp
auto handler = std::make_shared<ThreadSafeHandler>();
udp_server server(std::move(sock), handler);
```

**Use when:**
- Handlers share state (with proper locking)
- Memory efficiency is critical
- Stateless packet processing

---

#### 3. Filtering Factory

```cpp
auto factory = [](const udp_server::PacketData& packet) {
    // Filter by sender IP
    if (packet.sender_address.host() == "192.168.1.100") {
        return std::unique_ptr<MyHandler>(nullptr);  // Drop packet
    }
    return std::make_unique<MyHandler>(packet);
};
```

---

#### 4. Protocol-Based Routing

```cpp
auto factory = [](const udp_server::PacketData& packet) {
    // Inspect packet data to determine handler type
    if (packet.buffer.size() > 0 && packet.buffer[0] == 0x01) {
        return std::make_unique<Protocol1Handler>(packet);
    } else if (packet.buffer.size() > 0 && packet.buffer[0] == 0x02) {
        return std::make_unique<Protocol2Handler>(packet);
    }
    return std::make_unique<DefaultHandler>(packet);
};
```

---

#### 5. Context-Aware Factory

```cpp
class HandlerFactoryWithContext {
    Database& db;
public:
    HandlerFactoryWithContext(Database& database) : db(database) {}

    std::unique_ptr<udp_handler> operator()(const udp_server::PacketData& packet) {
        return std::make_unique<DatabaseHandler>(packet, db);
    }
};

HandlerFactoryWithContext factory(my_database);
server.set_handler_factory(factory);
```

---

## Threading Model

### Architecture Overview

```
Main Thread              Receiver Thread              Worker Thread Pool
    |                           |                              |
start() ----------------> receive loop                wait for packets
    |                           |                              |
    |                    [new packet]                          |
    |                           |                              |
    |                  create PacketData                       |
    |                           |                              |
    |                    queue packet ------------------------>+
    |                           |                              |
    |                  notify workers -----------------------> dequeue
    |                           |                              |
    |                  add thread if needed              create handler
    |                           |                              |
    |                           |                         handle_packet()
    |                           |                              |
    |                           |                         increment stats
    |                           |                              |
    |                           |                         cleanup & repeat
```

### Thread Pools

#### Receiver Thread
- **Count:** 1
- **Purpose:** Receive incoming UDP packets
- **Behavior:**
  - Calls `udp_socket::receive_from()` in a loop
  - Creates `PacketData` structure
  - Queues packet for worker processing
  - Adds worker threads dynamically if needed
  - Drops packets if queue is full

#### Worker Thread Pool
- **Initial Count:** Variable (starts with 1-4 threads)
- **Maximum Count:** Configurable (default 10)
- **Scaling:** Automatic based on queue depth
- **Behavior:**
  - Wait on condition variable for queued packets
  - Dequeue packet
  - Create handler via factory (or use shared handler)
  - Call `handler->handle_packet()`
  - Update statistics
  - Repeat

### Thread Scaling

The server automatically adds worker threads when packet queue grows:

```cpp
// Add worker thread if:
if (packet_queue.size() > worker_threads.size() &&
    worker_threads.size() < max_threads) {
    worker_threads.emplace_back(worker_thread_proc);
}
```

---

## Packet Data Structure

### PacketData

```cpp
struct PacketData {
    std::vector<std::uint8_t> buffer;          // Packet contents
    socket_address sender_address;              // Sender's address
    std::chrono::steady_clock::time_point received_time;  // Reception timestamp

    PacketData(const void* data, std::size_t length, const socket_address& sender);
};
```

**Members:**
- `buffer` - Packet data as byte vector
- `sender_address` - Source address of the packet
- `received_time` - Timestamp when packet was received

**Usage in Factory:**
```cpp
auto factory = [](const udp_server::PacketData& packet) {
    auto elapsed = std::chrono::steady_clock::now() - packet.received_time;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    std::cout << "Processing packet from " << packet.sender_address.to_string()
              << " (queued for " << ms << "ms)\n";

    return std::make_unique<MyHandler>(packet);
};
```

---

## Complete Examples

### Example 1: Echo Server with Statistics

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <atomic>

using namespace fb;

class EchoHandler : public udp_handler {
    udp_socket& m_reply_socket;
public:
    EchoHandler(udp_socket& reply_socket) : m_reply_socket(reply_socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        // Echo the packet back
        m_reply_socket.send_to(buffer, length, sender_address);

        std::cout << "[ECHO] " << sender_address.to_string()
                  << " - " << length << " bytes\n";
    }
};

int main() {
    fb::initialize_library();

    // Create and bind socket
    udp_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 8888));

    // Need socket reference for replies
    udp_socket& sock_ref = sock;

    // Factory that creates handlers with socket reference
    auto factory = [&sock_ref](const udp_server::PacketData& packet) {
        return std::make_unique<EchoHandler>(sock_ref);
    };

    udp_server server(std::move(sock), factory);

    // Attach statistics signals
    server.onTotalPacketsChanged.connect([](size_t total) {
        if (total % 100 == 0) {
            std::cout << "[STATS] Total: " << total << "\n";
        }
    });

    server.onDroppedPacketsChanged.connect([](size_t dropped) {
        std::cerr << "[WARNING] Dropped: " << dropped << "\n";
    });

    server.start();

    std::cout << "UDP Echo Server listening on port 8888\n";
    std::cout << "Press Enter to stop...\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Ping-Pong Server

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <sstream>
#include <chrono>

using namespace fb;

class PingPongHandler : public udp_handler {
    udp_socket& m_socket;
public:
    PingPongHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string request(static_cast<const char*>(buffer), length);

        if (request.find("PING") == 0) {
            // Extract sequence number
            std::istringstream iss(request);
            std::string cmd;
            int seq;
            iss >> cmd >> seq;

            // Create PONG response
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now).count();

            std::ostringstream response;
            response << "PONG " << seq << " " << timestamp;

            std::string response_str = response.str();
            m_socket.send_to(response_str.c_str(), response_str.length(), sender_address);

            std::cout << "[PING] seq=" << seq << " from "
                      << sender_address.to_string() << "\n";
        }
    }
};

int main() {
    fb::initialize_library();

    udp_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 7777));

    udp_socket& sock_ref = sock;

    auto factory = [&sock_ref](const udp_server::PacketData& packet) {
        return std::make_unique<PingPongHandler>(sock_ref);
    };

    udp_server server(std::move(sock), factory, 10, 500);
    server.start();

    std::cout << "Ping-Pong Server on port 7777\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 3: Protocol Server with Filtering

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <unordered_set>
#include <mutex>

using namespace fb;

// IP whitelist
std::unordered_set<std::string> g_whitelist = {
    "127.0.0.1", "192.168.1.100", "10.0.0.50"
};
std::mutex g_whitelist_mutex;

class ProtocolHandler : public udp_handler {
    udp_socket& m_socket;
public:
    ProtocolHandler(udp_socket& socket) : m_socket(socket) {}

    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string data(static_cast<const char*>(buffer), length);

        // Process protocol commands
        if (data.find("GET_STATUS") == 0) {
            std::string response = "STATUS OK\n";
            m_socket.send_to(response.c_str(), response.length(), sender_address);
        }
        else if (data.find("GET_TIME") == 0) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::string response = "TIME " + std::string(std::ctime(&time_t));
            m_socket.send_to(response.c_str(), response.length(), sender_address);
        }
        else {
            std::string response = "ERROR Unknown command\n";
            m_socket.send_to(response.c_str(), response.length(), sender_address);
        }
    }
};

int main() {
    fb::initialize_library();

    udp_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 9999));

    udp_socket& sock_ref = sock;

    // Factory with whitelist filtering
    auto factory = [&sock_ref](const udp_server::PacketData& packet)
        -> std::unique_ptr<udp_handler>
    {
        std::string client_ip = packet.sender_address.host();

        // Check whitelist
        {
            std::lock_guard<std::mutex> lock(g_whitelist_mutex);
            if (g_whitelist.count(client_ip) == 0) {
                std::cout << "[BLOCKED] " << client_ip << "\n";
                return nullptr;  // Drop packet
            }
        }

        std::cout << "[ACCEPTED] " << client_ip << "\n";
        return std::make_unique<ProtocolHandler>(sock_ref);
    };

    udp_server server(std::move(sock), factory, 20, 1000);
    server.start();

    std::cout << "Protocol Server on port 9999 (whitelist enabled)\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 4: Multicast Server

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

class MulticastHandler : public udp_handler {
public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::string message(static_cast<const char*>(buffer), length);
        std::cout << "[MULTICAST] From " << sender_address.to_string()
                  << ": " << message << "\n";
    }
};

int main() {
    fb::initialize_library();

    // Create socket and join multicast group
    udp_socket sock(socket_address::IPv4);
    sock.set_reuse_address(true);
    sock.bind(socket_address("0.0.0.0", 5555));
    sock.join_group(socket_address("239.255.0.1"));

    auto handler = std::make_shared<MulticastHandler>();

    udp_server server(std::move(sock), handler);
    server.start();

    std::cout << "Multicast Server listening on 239.255.0.1:5555\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 5: Server with Custom Virtual Hooks

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <fstream>

using namespace fb;

class LoggingUDPServer : public udp_server {
    std::ofstream m_log;
public:
    using udp_server::udp_server;

    bool open_log(const std::string& filename) {
        m_log.open(filename, std::ios::app);
        return m_log.is_open();
    }

protected:
    void on_server_starting() override {
        m_log << "[" << timestamp() << "] Server starting\n";
        m_log.flush();
    }

    void on_server_stopping() override {
        m_log << "[" << timestamp() << "] Server stopping\n";
        m_log.flush();
    }

    void handle_exception(const std::exception& ex,
                         const std::string& context) noexcept override {
        m_log << "[" << timestamp() << "] ERROR in " << context
              << ": " << ex.what() << "\n";
        m_log.flush();
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

int main() {
    fb::initialize_library();

    udp_socket sock(socket_address::IPv4);
    sock.bind(socket_address("0.0.0.0", 8888));

    auto handler = std::make_shared<MyHandler>();

    LoggingUDPServer server(std::move(sock), handler);
    if (!server.open_log("udp_server.log")) {
        std::cerr << "Failed to open log file\n";
        return 1;
    }

    server.start();

    std::cout << "Logging UDP Server on port 8888\n";
    std::cout << "Check udp_server.log for activity\n";
    std::cin.get();

    server.stop();
    fb::cleanup_library();
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Packet Rate Limiting

```cpp
class RateLimitedHandler : public udp_handler {
    std::unordered_map<std::string, std::deque<std::chrono::steady_clock::time_point>> m_requests;
    std::mutex m_mutex;
    size_t m_max_per_second = 10;

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string client_ip = sender_address.host();

        auto now = std::chrono::steady_clock::now();
        auto& times = m_requests[client_ip];

        // Remove old entries
        while (!times.empty() && now - times.front() > std::chrono::seconds(1)) {
            times.pop_front();
        }

        if (times.size() >= m_max_per_second) {
            std::cout << "[RATE_LIMIT] " << client_ip << " exceeded limit\n";
            return;  // Drop packet
        }

        times.push_back(now);

        // Process packet normally...
    }
};
```

---

### Pattern 2: Packet Aggregation

```cpp
class AggregatingHandler : public udp_handler {
    std::vector<std::vector<uint8_t>> m_batch;
    std::mutex m_mutex;
    size_t m_batch_size = 100;

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_batch.emplace_back(static_cast<const uint8_t*>(buffer),
                            static_cast<const uint8_t*>(buffer) + length);

        if (m_batch.size() >= m_batch_size) {
            process_batch();
            m_batch.clear();
        }
    }

    void process_batch() {
        std::cout << "[BATCH] Processing " << m_batch.size() << " packets\n";
        // Process aggregated packets...
    }
};
```

---

### Pattern 3: Stateful Session Management

```cpp
struct Session {
    socket_address address;
    std::chrono::steady_clock::time_point last_activity;
    uint64_t packet_count = 0;
};

class SessionHandler : public udp_handler {
    std::unordered_map<std::string, Session> m_sessions;
    std::mutex m_mutex;

public:
    void handle_packet(const void* buffer, std::size_t length,
                      const socket_address& sender_address) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::string key = sender_address.to_string();
        auto& session = m_sessions[key];
        session.address = sender_address;
        session.last_activity = std::chrono::steady_clock::now();
        session.packet_count++;

        std::cout << "[SESSION] " << key << " packet #"
                  << session.packet_count << "\n";

        // Process packet based on session state...
    }
};
```

---

## Error Handling

### Exception Types

**std::runtime_error:**
- Server already running when `start()` is called
- Server not running when `stop()` is called
- Configuration methods called while running
- Server not properly configured
- No socket or handler set

**std::system_error:**
- UDP socket is closed or invalid

---

### Virtual Exception Hook

```cpp
class MyUDPServer : public udp_server {
protected:
    void handle_exception(const std::exception& ex,
                         const std::string& context) noexcept override
    {
        // Log and handle exceptions
        log_error(context, ex.what());

        // Take action based on context
        if (context == "receiver_thread") {
            // Critical - receiver thread failed
            alert_admin("Receiver thread crashed");
        }
    }
};
```

---

## Performance Considerations

### Thread Pool Sizing

**UDP Workload Guidelines:**
- **Packet inspection:** 5-10 threads
- **Simple processing:** 10-20 threads
- **Database queries:** 20-50 threads
- **Heavy computation:** Number of CPU cores

```cpp
// Light processing
server.set_max_threads(10);

// Heavy processing
server.set_max_threads(50);
```

---

### Queue Depth Tuning

```cpp
// Low latency (drop excess)
server.set_max_queued(100);

// High reliability (buffer bursts)
server.set_max_queued(10000);
```

---

### Buffer Size Optimization

```cpp
// Standard Ethernet MTU
server.set_packet_buffer_size(1500);

// Jumbo frames
server.set_packet_buffer_size(9000);

// Maximum UDP
server.set_packet_buffer_size(65507);
```

---

## Platform-Specific Notes

### Windows
- Uses Winsock2 (initialized via `fb::initialize_library()`)
- Thread limit depends on available memory

### Linux
- Check file descriptor limits: `ulimit -n`
- Increase if needed: `ulimit -n 65536`

### macOS/BSD
- Similar to Linux
- File descriptor limits apply

---

## See Also

- [udp_handler](udp_handler.md) - Base class for UDP packet handlers
- [udp_socket](udp_socket.md) - UDP socket for datagram communication
- [tcp_server](tcp_server.md) - Multi-threaded TCP server
- [socket_address](socket_address.md) - Network address representation
- [Signal](../fb_signal/doc/Signal.md) - Event notification system

---

**[Back to Index](index.md)** | **[Previous: tcp_server](tcp_server.md)** | **[Next: udp_handler](udp_handler.md)**
