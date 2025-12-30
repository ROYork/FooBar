# udp_client - High-Level UDP Client

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [Construction](#construction)
- [Connection Management](#connection-management)
- [Connected UDP Operations](#connected-udp-operations)
- [Connectionless UDP Operations](#connectionless-udp-operations)
- [Broadcast Operations](#broadcast-operations)
- [Timeout Configuration](#timeout-configuration)
- [Buffer Configuration](#buffer-configuration)
- [Status Queries](#status-queries)
- [Signal Events](#signal-events)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)
- [Error Handling](#error-handling)
- [See Also](#see-also)

---

## Overview

The `udp_client` class provides a high-level, simplified interface for UDP communication built on top of `udp_socket`. It supports both **connected** and **connectionless** operations, automatic addressing, timeouts, and broadcast/multicast capabilities with comprehensive signal-based event notification.

**Key Features:**
- **Simplified API** - Higher-level abstractions over raw socket operations
- **Connected Mode** - Associate client with a specific remote address
- **Connectionless Mode** - Explicitly specify destination for each send
- **Signal Events** - Real-time notifications for all operations
- **Timeout Support** - Configurable timeouts for send/receive operations
- **Broadcast Support** - Easy broadcast messaging
- **String API** - Convenient string-based send/receive methods
- **Buffer Configuration** - Adjustable send/receive buffer sizes
- **Move Semantics** - Efficient resource management

**Modes of Operation:**

1. **Connected Mode** (`connect()` called):
   - Socket associated with specific remote address
   - Use `send()` and `receive()` without address parameters
   - More convenient for client-server communication

2. **Connectionless Mode** (default):
   - Specify destination address for each send operation
   - Use `send_to()` and `receive_from()` with explicit addresses
   - More flexible for multi-peer communication

**Include:** `<fb/udp_client.h>`
**Namespace:** `fb`
**Platform Support:** Windows, macOS, Linux, BSD

---

## Quick Start

### Connected Mode Example

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    // Create UDP client
    udp_client client(socket_address::IPv4);

    // Connect to server
    client.connect(socket_address("127.0.0.1", 8888));

    // Send data (no address needed - connected mode)
    client.send("Hello, Server!");

    // Receive response
    std::string response;
    client.receive(response, 1024);

    std::cout << "Server replied: " << response << "\n";

    client.close();
    fb::cleanup_library();
    return 0;
}
```

### Connectionless Mode Example

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    // Create and bind client
    udp_client client(socket_address::IPv4);
    client.bind(socket_address("0.0.0.0", 0));  // Bind to any port

    socket_address server_addr("127.0.0.1", 8888);

    // Send to specific address
    client.send_to("Hello, Server!", server_addr);

    // Receive from any sender
    std::string response;
    socket_address sender;
    client.receive_from(response, 1024, sender);

    std::cout << "Received from " << sender.to_string()
              << ": " << response << "\n";

    client.close();
    fb::cleanup_library();
    return 0;
}
```

---

## Construction

### Default Constructor

```cpp
udp_client();
```

Creates a UDP client with IPv4 family and default configuration.

**Defaults:**
- Family: IPv4
- Timeout: 30000ms (30 seconds)
- Not connected
- Not bound

**Example:**
```cpp
udp_client client;  // IPv4, not connected
```

---

### Family Constructor

```cpp
explicit udp_client(socket_address::Family family);
```

Creates a UDP client with specified address family.

**Parameters:**
- `family` - `socket_address::IPv4` or `socket_address::IPv6`

**Example:**
```cpp
udp_client client(socket_address::IPv6);  // IPv6 client
```

---

### Local Address Constructor

```cpp
explicit udp_client(const socket_address& local_address, bool reuse_address = true);
```

Creates and binds a UDP client to a local address.

**Parameters:**
- `local_address` - Local endpoint to bind to
- `reuse_address` - Enable SO_REUSEADDR (default: true)

**Example:**
```cpp
// Bind to specific port
udp_client client(socket_address("0.0.0.0", 5555));

// Bind to any port
udp_client client(socket_address("0.0.0.0", 0));
```

---

### Connected Constructor

```cpp
udp_client(const socket_address& remote_address,
          const socket_address& local_address = socket_address());
```

Creates a UDP client and connects it to a remote address.

**Parameters:**
- `remote_address` - Remote endpoint to connect to
- `local_address` - Optional local endpoint to bind to (default: any)

**Example:**
```cpp
// Connect to server
udp_client client(socket_address("192.168.1.100", 8888));

// Connect with specific local address
udp_client client(
    socket_address("192.168.1.100", 8888),  // Remote
    socket_address("0.0.0.0", 5555)         // Local
);
```

---

### Move Semantics

```cpp
udp_client(udp_client&& other) noexcept;
udp_client& operator=(udp_client&& other) noexcept;
```

UDP client is **move-only** (no copy operations).

**Example:**
```cpp
udp_client create_client() {
    udp_client client(socket_address::IPv4);
    client.connect(socket_address("127.0.0.1", 8888));
    return client;  // Move construction
}

udp_client client = create_client();  // Moved
```

---

## Connection Management

### connect()

```cpp
void connect(const socket_address& address);
```

Connects the client to a specific remote address. After connecting, use `send()` and `receive()` without address parameters.

**Parameters:**
- `address` - Remote endpoint to connect to

**Behavior:**
- Sets connected mode
- Stores remote address
- Emits `onConnected` signal

**Throws:** `std::system_error` on failure

**Example:**
```cpp
udp_client client(socket_address::IPv4);
client.connect(socket_address("server.example.com", 8888));

// Now connected - can use send() without address
client.send("Hello!");
```

---

### bind()

```cpp
void bind(const socket_address& address, bool reuse_address = true);
```

Binds the client to a local address.

**Parameters:**
- `address` - Local endpoint to bind to
- `reuse_address` - Enable SO_REUSEADDR (default: true)

**Throws:** `std::system_error` on failure

**Example:**
```cpp
udp_client client(socket_address::IPv4);
client.bind(socket_address("0.0.0.0", 5555));
```

---

### disconnect()

```cpp
void disconnect();
```

Disconnects the client from the remote address (returns to connectionless mode).

**Behavior:**
- Clears connected mode flag
- Clears remote address
- Emits `onDisconnected` signal
- Socket remains open

**Example:**
```cpp
client.connect(socket_address("127.0.0.1", 8888));
// ... use connected mode ...
client.disconnect();
// Now in connectionless mode
```

---

### close()

```cpp
void close();
```

Closes the underlying UDP socket.

**Behavior:**
- Closes socket
- Clears connected state
- Socket cannot be used after close

**Example:**
```cpp
client.close();
// Client is now closed
```

---

## Connected UDP Operations

These methods are used when the client is connected to a remote address via `connect()`.

### send() - Buffer

```cpp
int send(const void* buffer, std::size_t length);
```

Sends data to the connected remote address.

**Parameters:**
- `buffer` - Data to send
- `length` - Number of bytes to send

**Returns:** Number of bytes sent

**Throws:** `std::system_error` if not connected or on error

**Emits:** `onDataSent` signal on success

**Example:**
```cpp
client.connect(socket_address("127.0.0.1", 8888));

const char* data = "Hello";
int sent = client.send(data, 5);
std::cout << "Sent " << sent << " bytes\n";
```

---

### send() - String

```cpp
int send(const std::string& message);
```

Sends a string to the connected remote address.

**Parameters:**
- `message` - String to send

**Returns:** Number of bytes sent

**Throws:** `std::system_error` if not connected or on error

**Emits:** `onDataSent` signal on success

**Example:**
```cpp
client.connect(socket_address("127.0.0.1", 8888));

int sent = client.send("Hello, Server!");
std::cout << "Sent " << sent << " bytes\n";
```

---

### receive() - Buffer

```cpp
int receive(void* buffer, std::size_t length);
```

Receives data from the connected remote address.

**Parameters:**
- `buffer` - Buffer to receive data into
- `length` - Maximum bytes to receive

**Returns:** Number of bytes received, or 0 on timeout

**Throws:** `std::system_error` on error

**Emits:** `onDataReceived` signal on success

**Example:**
```cpp
char buffer[1024];
int received = client.receive(buffer, sizeof(buffer));
if (received > 0) {
    std::cout << "Received " << received << " bytes\n";
}
```

---

### receive() - String

```cpp
int receive(std::string& message, std::size_t max_length = 1024);
```

Receives data into a string from the connected remote address.

**Parameters:**
- `message` - String to receive data into (output)
- `max_length` - Maximum bytes to receive (default: 1024)

**Returns:** Number of bytes received, or 0 on timeout

**Throws:** `std::system_error` on error

**Emits:** `onDataReceived` signal on success

**Example:**
```cpp
std::string response;
int received = client.receive(response, 4096);
if (received > 0) {
    std::cout << "Received: " << response << "\n";
}
```

---

## Connectionless UDP Operations

These methods explicitly specify the destination address for each operation.

### send_to() - Buffer

```cpp
int send_to(const void* buffer, std::size_t length, const socket_address& address);
```

Sends data to a specific address.

**Parameters:**
- `buffer` - Data to send
- `length` - Number of bytes
- `address` - Destination address

**Returns:** Number of bytes sent

**Throws:** `std::system_error` on error

**Emits:** `onDataSent` signal

**Example:**
```cpp
udp_client client(socket_address::IPv4);

const char* data = "Hello";
socket_address dest("192.168.1.100", 8888);

int sent = client.send_to(data, 5, dest);
```

---

### send_to() - String

```cpp
int send_to(const std::string& message, const socket_address& address);
```

Sends a string to a specific address.

**Parameters:**
- `message` - String to send
- `address` - Destination address

**Returns:** Number of bytes sent

**Throws:** `std::system_error` on error

**Example:**
```cpp
socket_address server("server.example.com", 8888);
int sent = client.send_to("Hello, Server!", server);
```

---

### receive_from() - Buffer

```cpp
int receive_from(void* buffer, std::size_t length, socket_address& sender_address);
```

Receives data and captures the sender's address.

**Parameters:**
- `buffer` - Buffer to receive into
- `length` - Maximum bytes
- `sender_address` - Output parameter for sender's address

**Returns:** Number of bytes received

**Throws:** `std::system_error` on error

**Emits:** `onDatagramReceived` signal with sender address

**Example:**
```cpp
char buffer[1024];
socket_address sender;

int received = client.receive_from(buffer, sizeof(buffer), sender);
std::cout << "Received " << received << " bytes from "
          << sender.to_string() << "\n";
```

---

### receive_from() - String

```cpp
int receive_from(std::string& message, std::size_t max_length, socket_address& sender_address);
```

Receives data into a string and captures the sender's address.

**Parameters:**
- `message` - String to receive into (output)
- `max_length` - Maximum bytes
- `sender_address` - Output parameter for sender's address

**Returns:** Number of bytes received

**Throws:** `std::system_error` on error

**Example:**
```cpp
std::string message;
socket_address sender;

int received = client.receive_from(message, 4096, sender);
std::cout << "From " << sender.to_string() << ": " << message << "\n";
```

---

### send_with_timeout()

```cpp
int send_with_timeout(const void* buffer, std::size_t length,
                     const std::chrono::milliseconds& timeout);
```

Sends data with a specific timeout (connected mode).

**Parameters:**
- `buffer` - Data to send
- `length` - Number of bytes
- `timeout` - Send timeout

**Returns:** Number of bytes sent

**Throws:** ``std::system_error` (std::errc::timed_out)` on timeout, `std::system_error` on error

**Example:**
```cpp
client.connect(socket_address("127.0.0.1", 8888));

const char* data = "Hello";
int sent = client.send_with_timeout(data, 5, std::chrono::seconds(5));
```

---

### receive_with_timeout()

```cpp
int receive_with_timeout(void* buffer, std::size_t length,
                        const std::chrono::milliseconds& timeout);
```

Receives data with a specific timeout.

**Parameters:**
- `buffer` - Buffer to receive into
- `length` - Maximum bytes
- `timeout` - Receive timeout

**Returns:** Number of bytes received, 0 on timeout

**Throws:** `std::system_error` on error (not timeout)

**Example:**
```cpp
char buffer[1024];
int received = client.receive_with_timeout(buffer, sizeof(buffer),
                                          std::chrono::seconds(10));
if (received == 0) {
    std::cout << "Receive timeout\n";
}
```

---

## Broadcast Operations

### set_broadcast()

```cpp
void set_broadcast(bool flag);
```

Enables or disables broadcast mode (SO_BROADCAST).

**Parameters:**
- `flag` - true to enable broadcast, false to disable

**Throws:** `std::system_error` on error

**Example:**
```cpp
client.set_broadcast(true);  // Enable broadcast
```

---

### get_broadcast()

```cpp
bool get_broadcast();
```

Gets the current broadcast mode setting.

**Returns:** true if broadcast enabled, false otherwise

**Example:**
```cpp
if (client.get_broadcast()) {
    std::cout << "Broadcast enabled\n";
}
```

---

### broadcast() - Buffer

```cpp
int broadcast(const void* buffer, std::size_t length, std::uint16_t port);
```

Sends broadcast message to all hosts on the local network.

**Parameters:**
- `buffer` - Data to broadcast
- `length` - Number of bytes
- `port` - Destination port

**Returns:** Number of bytes sent

**Throws:** `std::system_error` on error

**Emits:** `onBroadcastSent` signal

**Example:**
```cpp
client.set_broadcast(true);

const char* msg = "DISCOVER";
int sent = client.broadcast(msg, 8, 8888);  // Broadcast to port 8888
```

---

### broadcast() - String

```cpp
int broadcast(const std::string& message, std::uint16_t port);
```

Sends broadcast string message.

**Parameters:**
- `message` - String to broadcast
- `port` - Destination port

**Returns:** Number of bytes sent

**Throws:** `std::system_error` on error

**Example:**
```cpp
client.set_broadcast(true);
int sent = client.broadcast("Service Discovery", 9999);
```

---

## Timeout Configuration

### set_timeout()

```cpp
void set_timeout(const std::chrono::milliseconds& timeout);
```

Sets the default timeout for send/receive operations.

**Parameters:**
- `timeout` - Timeout duration (0 = no timeout)

**Default:** 30000ms (30 seconds)

**Example:**
```cpp
client.set_timeout(std::chrono::seconds(10));  // 10 second timeout
```

---

### get_timeout()

```cpp
std::chrono::milliseconds get_timeout();
```

Gets the current timeout setting.

**Returns:** Current timeout duration

**Example:**
```cpp
auto timeout = client.get_timeout();
std::cout << "Timeout: " << timeout.count() << "ms\n";
```

---

## Buffer Configuration

### set_receive_buffer_size()

```cpp
void set_receive_buffer_size(int size);
```

Sets the size of the receive buffer (SO_RCVBUF).

**Parameters:**
- `size` - Buffer size in bytes

**Throws:** `std::system_error` on error

**Example:**
```cpp
client.set_receive_buffer_size(65536);  // 64KB receive buffer
```

---

### get_receive_buffer_size()

```cpp
int get_receive_buffer_size();
```

Gets the current receive buffer size.

**Returns:** Buffer size in bytes

**Example:**
```cpp
int size = client.get_receive_buffer_size();
std::cout << "RX buffer: " << size << " bytes\n";
```

---

### set_send_buffer_size()

```cpp
void set_send_buffer_size(int size);
```

Sets the size of the send buffer (SO_SNDBUF).

**Parameters:**
- `size` - Buffer size in bytes

**Throws:** `std::system_error` on error

**Example:**
```cpp
client.set_send_buffer_size(65536);  // 64KB send buffer
```

---

### get_send_buffer_size()

```cpp
int get_send_buffer_size();
```

Gets the current send buffer size.

**Returns:** Buffer size in bytes

**Example:**
```cpp
int size = client.get_send_buffer_size();
std::cout << "TX buffer: " << size << " bytes\n";
```

---

## Status Queries

### is_connected()

```cpp
bool is_connected() const;
```

Checks if the client is connected to a remote address.

**Returns:** true if connected, false otherwise

**Example:**
```cpp
if (client.is_connected()) {
    client.send("Hello!");
} else {
    std::cout << "Not connected\n";
}
```

---

### is_closed()

```cpp
bool is_closed() const;
```

Checks if the underlying socket is closed.

**Returns:** true if closed, false otherwise

**Example:**
```cpp
if (!client.is_closed()) {
    client.send("Data");
}
```

---

### local_address()

```cpp
socket_address local_address() const;
```

Gets the local endpoint address.

**Returns:** Local socket_address

**Throws:** `std::system_error` if socket not bound

**Example:**
```cpp
std::cout << "Local address: " << client.local_address().to_string() << "\n";
```

---

### remote_address()

```cpp
socket_address remote_address() const;
```

Gets the connected remote endpoint address.

**Returns:** Remote socket_address

**Throws:** `std::runtime_error` if not connected

**Example:**
```cpp
if (client.is_connected()) {
    std::cout << "Connected to: " << client.remote_address().to_string() << "\n";
}
```

---

### has_data_available()

```cpp
bool has_data_available(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));
```

Checks if data is available for reading.

**Parameters:**
- `timeout` - Wait timeout (0 = check immediately)

**Returns:** true if data available, false otherwise

**Example:**
```cpp
if (client.has_data_available(std::chrono::seconds(1))) {
    std::string data;
    client.receive(data, 1024);
}
```

---

### socket()

```cpp
udp_socket& socket();
const udp_socket& socket() const;
```

Accesses the underlying UDP socket for advanced operations.

**Returns:** Reference to the udp_socket

**Example:**
```cpp
// Access advanced socket features
client.socket().join_group(socket_address("239.255.0.1"));
```

---

## Signal Events

### Connection Signals

#### onConnected

```cpp
fb::signal<const socket_address&> onConnected;
```

Emitted when client connects to a remote address.

**Parameters:**
- `const socket_address&` - Remote address connected to

**Example:**
```cpp
client.onConnected.connect([](const socket_address& addr) {
    std::cout << "Connected to " << addr.to_string() << "\n";
});

client.connect(socket_address("127.0.0.1", 8888));
```

---

#### onDisconnected

```cpp
fb::signal<> onDisconnected;
```

Emitted when client disconnects.

**Example:**
```cpp
client.onDisconnected.connect([]() {
    std::cout << "Disconnected\n";
});

client.disconnect();
```

---

### Data Transfer Signals

#### onDataSent

```cpp
fb::signal<const void*, std::size_t> onDataSent;
```

Emitted after successful send operation.

**Parameters:**
- `const void*` - Pointer to sent data
- `std::size_t` - Number of bytes sent

**Example:**
```cpp
client.onDataSent.connect([](const void* data, std::size_t len) {
    std::cout << "Sent " << len << " bytes\n";
});
```

---

#### onDataReceived

```cpp
fb::signal<const void*, std::size_t> onDataReceived;
```

Emitted after successful receive operation (connected mode).

**Parameters:**
- `const void*` - Pointer to received data
- `std::size_t` - Number of bytes received

**Example:**
```cpp
client.onDataReceived.connect([](const void* data, std::size_t len) {
    std::string msg(static_cast<const char*>(data), len);
    std::cout << "Received: " << msg << "\n";
});
```

---

#### onDatagramReceived

```cpp
fb::signal<const void*, std::size_t, const socket_address&> onDatagramReceived;
```

Emitted when datagram is received (connectionless mode).

**Parameters:**
- `const void*` - Pointer to data
- `std::size_t` - Number of bytes
- `const socket_address&` - Sender's address

**Example:**
```cpp
client.onDatagramReceived.connect([](const void* data, std::size_t len, const socket_address& sender) {
    std::cout << "Datagram from " << sender.to_string() << ": " << len << " bytes\n";
});
```

---

### Broadcast Signals

#### onBroadcastSent

```cpp
fb::signal<const void*, std::size_t> onBroadcastSent;
```

Emitted after successful broadcast send.

**Example:**
```cpp
client.onBroadcastSent.connect([](const void* data, std::size_t len) {
    std::cout << "Broadcast sent: " << len << " bytes\n";
});
```

---

### Error Signals

#### onSendError

```cpp
fb::signal<const std::string&> onSendError;
```

Emitted on send failure.

**Example:**
```cpp
client.onSendError.connect([](const std::string& error) {
    std::cerr << "Send error: " << error << "\n";
});
```

---

#### onReceiveError

```cpp
fb::signal<const std::string&> onReceiveError;
```

Emitted on receive failure.

**Example:**
```cpp
client.onReceiveError.connect([](const std::string& error) {
    std::cerr << "Receive error: " << error << "\n";
});
```

---

#### onTimeoutError

```cpp
fb::signal<const std::string&> onTimeoutError;
```

Emitted on timeout.

**Example:**
```cpp
client.onTimeoutError.connect([](const std::string& error) {
    std::cout << "Timeout: " << error << "\n";
});
```

---

## Complete Examples

### Example 1: Simple Request-Response Client

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    udp_client client(socket_address::IPv4);
    client.set_timeout(std::chrono::seconds(5));

    // Connect to server
    client.connect(socket_address("127.0.0.1", 8888));

    // Send request
    client.send("GET_TIME");

    // Receive response
    std::string response;
    int received = client.receive(response, 1024);

    if (received > 0) {
        std::cout << "Server time: " << response << "\n";
    } else {
        std::cout << "No response (timeout)\n";
    }

    client.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Broadcast Discovery Client

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <thread>

using namespace fb;

int main() {
    fb::initialize_library();

    udp_client client(socket_address::IPv4);
    client.bind(socket_address("0.0.0.0", 0));  // Bind to any port
    client.set_broadcast(true);
    client.set_timeout(std::chrono::seconds(2));

    // Send broadcast discovery
    std::cout << "Broadcasting discovery request...\n";
    client.broadcast("DISCOVER", 9999);

    // Collect responses
    std::vector<socket_address> servers;
    for (int i = 0; i < 5; i++) {
        std::string response;
        socket_address sender;

        int received = client.receive_from(response, 1024, sender);
        if (received > 0 && response == "SERVER_HERE") {
            servers.push_back(sender);
            std::cout << "Found server at " << sender.to_string() << "\n";
        } else {
            break;  // Timeout
        }
    }

    std::cout << "Discovered " << servers.size() << " servers\n";

    client.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 3: Signal-Driven UDP Client

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <atomic>

using namespace fb;

int main() {
    fb::initialize_library();

    udp_client client(socket_address::IPv4);

    std::atomic<size_t> bytes_sent{0};
    std::atomic<size_t> bytes_received{0};

    // Attach signal handlers
    client.onConnected.connect([](const socket_address& addr) {
        std::cout << "[EVENT] Connected to " << addr.to_string() << "\n";
    });

    client.onDataSent.connect([&bytes_sent](const void* data, std::size_t len) {
        bytes_sent += len;
        std::cout << "[TX] Sent " << len << " bytes (total: " << bytes_sent << ")\n";
    });

    client.onDataReceived.connect([&bytes_received](const void* data, std::size_t len) {
        bytes_received += len;
        std::string msg(static_cast<const char*>(data), len);
        std::cout << "[RX] Received: " << msg << " (total: " << bytes_received << " bytes)\n";
    });

    client.onSendError.connect([](const std::string& error) {
        std::cerr << "[ERROR] Send failed: " << error << "\n";
    });

    // Connect and communicate
    client.connect(socket_address("127.0.0.1", 8888));
    client.send("Hello, Server!");

    std::string response;
    client.receive(response, 1024);

    std::cout << "\nStatistics:\n";
    std::cout << "  Sent: " << bytes_sent << " bytes\n";
    std::cout << "  Received: " << bytes_received << " bytes\n";

    client.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 4: Multi-Server Client

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <vector>

using namespace fb;

int main() {
    fb::initialize_library();

    std::vector<socket_address> servers = {
        socket_address("192.168.1.10", 8888),
        socket_address("192.168.1.11", 8888),
        socket_address("192.168.1.12", 8888)
    };

    udp_client client(socket_address::IPv4);
    client.set_timeout(std::chrono::seconds(2));

    std::string request = "PING";

    for (const auto& server : servers) {
        std::cout << "Pinging " << server.to_string() << "...\n";

        client.send_to(request, server);

        std::string response;
        socket_address responder;
        int received = client.receive_from(response, 1024, responder);

        if (received > 0 && response == "PONG") {
            std::cout << "  ✓ Server is alive\n";
        } else {
            std::cout << "  ✗ No response\n";
        }
    }

    client.close();
    fb::cleanup_library();
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Request with Retry

```cpp
std::string request_with_retry(udp_client& client,
                               const std::string& request,
                               int max_retries = 3)
{
    client.set_timeout(std::chrono::seconds(2));

    for (int i = 0; i < max_retries; i++) {
        client.send(request);

        std::string response;
        int received = client.receive(response, 1024);

        if (received > 0) {
            return response;  // Success
        }

        std::cout << "Retry " << (i + 1) << "/" << max_retries << "...\n";
    }

    throw std::runtime_error("Max retries exceeded");
}
```

---

### Pattern 2: Ping-Pong Pattern

```cpp
void ping_pong(udp_client& client, int count) {
    client.set_timeout(std::chrono::seconds(5));

    for (int i = 0; i < count; i++) {
        auto start = std::chrono::steady_clock::now();

        // Send ping
        std::ostringstream ping;
        ping << "PING " << i;
        client.send(ping.str());

        // Receive pong
        std::string pong;
        int received = client.receive(pong, 1024);

        auto rtt = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(rtt).count();

        if (received > 0) {
            std::cout << "seq=" << i << " RTT=" << ms << "ms\n";
        } else {
            std::cout << "seq=" << i << " timeout\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

---

## Error Handling

### Exception Handling

```cpp
try {
    udp_client client(socket_address::IPv4);
    client.connect(socket_address("server.example.com", 8888));
    client.send("Hello!");

    std::string response;
    client.receive(response, 1024);

} catch (const `std::system_error` (std::errc::host_unreachable)& ex) {
    std::cerr << "Server not found: " << ex.what() << "\n";
} catch (const `std::system_error` (std::errc::timed_out)& ex) {
    std::cerr << "Operation timed out: " << ex.what() << "\n";
} catch (const std::system_error& ex) {
    std::cerr << "Network error: " << ex.what() << "\n";
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
}
```

---

### Signal-Based Error Handling

```cpp
udp_client client(socket_address::IPv4);

client.onSendError.connect([](const std::string& error) {
    log_error("Send failed: " + error);
    metrics::increment("udp.send.errors");
});

client.onReceiveError.connect([](const std::string& error) {
    log_error("Receive failed: " + error);
    metrics::increment("udp.receive.errors");
});

client.onTimeoutError.connect([](const std::string& error) {
    log_warning("Timeout: " + error);
    metrics::increment("udp.timeouts");
});
```

---

## See Also

- [udp_socket](udp_socket.md) - Low-level UDP socket
- [udp_server](udp_server.md) - Multi-threaded UDP server
- [socket_address](socket_address.md) - Network address representation
- [Signal](../fb_signal/doc/Signal.md) - Event notification system
- [Exception Handling](exceptions.md) - Network exception hierarchy

---

**[Back to Index](index.md)** | **[Previous: udp_handler](udp_handler.md)** | **[Next: socket_base](socket_base.md)**
