# fb::udp_socket - UDP Datagram Socket

## Overview

The [`fb::udp_socket`](../include/fb/udp_socket.h) class provides a high-level interface for UDP (User Datagram Protocol) communication. It inherits from `socket_base` and adds UDP-specific functionality including datagram operations, broadcast, and multicast support.

**Key Features:**
- Connectionless datagram communication
- Send/receive with explicit addressing
- Optional "connected" mode for single peer
- Broadcast support
- Multicast group management
- No connection overhead
- Suitable for real-time applications
- Exception-safe RAII design

**Namespace:** `fb`

**Header:** `#include <fb/udp_socket.h>`

---

## Quick Start

```cpp
#include <fb/udp_socket.h>
#include <iostream>

using namespace fb;

int main()
{
    try {
        // Create UDP socket
        udp_socket socket(socket_address::IPv4);

        // Bind to port
        socket.bind(socket_address(5000));
        std::cout << "UDP server listening on port 5000" << std::endl;

        // Receive datagram
        char buffer[1024];
        socket_address sender;
        int bytes = socket.receive_from(buffer, sizeof(buffer), sender);

        std::cout << "Received " << bytes << " bytes from "
                  << sender.to_string() << std::endl;

        // Send response
        socket.send_to("ACK", 3, sender);

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
```

---

## Construction

### Default Constructor

Creates an unbound IPv4 UDP socket.

```cpp
udp_socket();
```

**Example:**
```cpp
udp_socket socket;
socket.bind(socket_address(5000));
```

---

### Family Constructor

Creates an unbound UDP socket with specified address family.

```cpp
explicit udp_socket(socket_address::Family family);
```

**Parameters:**
- `family` - Address family (IPv4 or IPv6)

**Example:**
```cpp
// IPv4 socket
udp_socket socket_v4(socket_address::IPv4);

// IPv6 socket
udp_socket socket_v6(socket_address::IPv6);
```

---

### Address Constructor

Creates and binds to the specified address.

```cpp
explicit udp_socket(const socket_address& address, bool reuse_address = true);
```

**Parameters:**
- `address` - Address to bind to
- `reuse_address` - Enable SO_REUSEADDR (default: true)

**Example:**
```cpp
// Bind to port 5000 on all interfaces
udp_socket socket(socket_address(5000));

// Bind to specific interface
udp_socket socket(socket_address("192.168.1.100", 5000));
```

---

### Socket Descriptor Constructor

Creates from an existing socket descriptor.

```cpp
explicit udp_socket(socket_t sockfd);
```

**Parameters:**
- `sockfd` - Existing socket file descriptor

---

### Move Constructor

```cpp
udp_socket(udp_socket&& other) noexcept;
```

---

## Datagram Operations

### send_to()

Sends a datagram to a specific address.

```cpp
int send_to(const void* buffer, int length, const socket_address& address, int flags = 0);
int send_to(const std::string& message, const socket_address& address);
```

**Parameters:**
- `buffer` - Data to send
- `length` - Number of bytes to send
- `message` - String message to send
- `address` - Destination address
- `flags` - Socket flags (default: 0)

**Returns:** Number of bytes sent

**Throws:** `std::system_error` on send error

**Example:**
```cpp
udp_socket socket(socket_address::IPv4);

// Send raw bytes
char data[100] = {...};
socket.send_to(data, sizeof(data), socket_address("192.168.1.1", 5000));

// Send string
socket.send_to("Hello, UDP!", socket_address("example.com", 5000));
```

---

### receive_from()

Receives a datagram and captures the sender's address.

```cpp
int receive_from(void* buffer, int length, socket_address& address, int flags = 0);
int receive_from(std::string& message, int max_length, socket_address& address);
```

**Parameters:**
- `buffer` - Buffer to receive into
- `length` / `max_length` - Maximum bytes to receive
- `message` - String to receive into
- `address` - Output parameter for sender's address
- `flags` - Socket flags

**Returns:** Number of bytes received (0 = no data in non-blocking mode)

**Example:**
```cpp
udp_socket socket(socket_address(5000));

// Receive into buffer
char buffer[1024];
socket_address sender;
int bytes = socket.receive_from(buffer, sizeof(buffer), sender);

std::cout << "Received " << bytes << " bytes from "
          << sender.to_string() << std::endl;

// Receive into string
std::string message;
socket_address from;
socket.receive_from(message, 1024, from);
```

---

## Connected Mode

UDP sockets can optionally "connect" to a specific peer address, allowing use of send/receive without specifying the address each time.

### connect()

Associates the socket with a specific peer address.

```cpp
void connect(const socket_address& address);
```

**Parameters:**
- `address` - Peer address to associate with

**Benefits:**
- Simplified send/receive (no address needed)
- OS filters datagrams from other sources
- Slightly better performance

**Example:**
```cpp
udp_socket socket;
socket.connect(socket_address("192.168.1.1", 5000));

// Now can use send/receive without addresses
socket.send("Hello");

std::string response;
socket.receive(response, 1024);
```

---

### send() / receive() (Connected Mode)

Send and receive using the connected peer address.

```cpp
int send_bytes(const void* buffer, int length, int flags = 0);
int send(const std::string& message);
int receive_bytes(void* buffer, int length, int flags = 0);
int receive(std::string& buffer, int max_length);
```

**Precondition:** Socket must be connected via `connect()`

**Example:**
```cpp
udp_socket socket;
socket.connect(socket_address("server.example.com", 5000));

// Send without specifying address
socket.send("REQUEST");

// Receive from connected peer only
std::string response;
socket.receive(response, 1024);
```

---

### is_connected()

Checks if socket is in connected mode.

```cpp
bool is_connected() const;
```

**Returns:** `true` if connected, `false` otherwise

---

## Broadcast Support

### set_broadcast() / get_broadcast()

Controls SO_BROADCAST socket option.

```cpp
void set_broadcast(bool flag);
bool get_broadcast();
```

**Parameters:**
- `flag` - `true` to enable broadcast, `false` to disable

**Use Case:**
- Send datagrams to all hosts on local network
- Service discovery protocols
- Network announcements

**Example:**
```cpp
udp_socket socket;
socket.set_broadcast(true);

// Send to broadcast address
socket.send_to("DISCOVER", 8, socket_address("255.255.255.255", 9999));

// Or subnet-specific broadcast
socket.send_to("ANNOUNCE", 8, socket_address("192.168.1.255", 9999));
```

---

## Multicast Support

### join_group()

Joins a multicast group to receive multicast datagrams.

```cpp
void join_group(const socket_address& group_address);
void join_group(const socket_address& group_address, const socket_address& interface_address);
```

**Parameters:**
- `group_address` - Multicast group address (e.g., "239.255.0.1")
- `interface_address` - Specific interface to join on (optional)

**Example:**
```cpp
udp_socket socket(socket_address(5000));

// Join multicast group on default interface
socket.join_group(socket_address("239.255.0.1", 5000));

// Join on specific interface
socket.join_group(
    socket_address("239.255.0.1", 5000),
    socket_address("192.168.1.100", 0)
);

// Now receive multicast datagrams
char buffer[1024];
socket_address sender;
socket.receive_from(buffer, sizeof(buffer), sender);
```

---

### leave_group()

Leaves a multicast group.

```cpp
void leave_group(const socket_address& group_address);
void leave_group(const socket_address& group_address, const socket_address& interface_address);
```

**Example:**
```cpp
socket.leave_group(socket_address("239.255.0.1", 5000));
```

---

### set_multicast_ttl() / get_multicast_ttl()

Controls multicast Time-To-Live (hop limit).

```cpp
void set_multicast_ttl(int ttl);
int get_multicast_ttl();
```

**Parameters:**
- `ttl` - TTL value (1 = local subnet, higher = more hops)

**Common Values:**
- `1` - Local subnet only
- `32` - Organizational/site-wide
- `255` - Internet-wide

**Example:**
```cpp
udp_socket socket;
socket.set_multicast_ttl(1);  // Local subnet only
socket.send_to("MULTICAST", 9, socket_address("239.255.0.1", 5000));
```

---

### set_multicast_loopback() / get_multicast_loopback()

Controls whether multicast datagrams are looped back to sender.

```cpp
void set_multicast_loopback(bool flag);
bool get_multicast_loopback();
```

**Example:**
```cpp
socket.set_multicast_loopback(false);  // Don't receive own multicasts
```

---

## Polling

### poll_read() / poll_write()

Checks if socket is ready for reading or writing.

```cpp
bool poll_read(const std::chrono::milliseconds& timeout);
bool poll_write(const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `timeout` - Maximum time to wait

**Returns:** `true` if ready, `false` if timeout

**Example:**
```cpp
udp_socket socket(socket_address(5000));

// Poll for incoming datagram
if (socket.poll_read(std::chrono::milliseconds(100))) {
    char buffer[1024];
    socket_address sender;
    socket.receive_from(buffer, sizeof(buffer), sender);
}
```

---

## Socket Properties

### max_datagram_size()

Returns the maximum datagram size.

```cpp
int max_datagram_size() const;
```

**Returns:** Maximum UDP datagram size (typically 65507 bytes)

**Note:** Practical MTU limits are usually much smaller (1400-1500 bytes)

---

## Complete Examples

### Example 1: UDP Echo Server

```cpp
#include <fb/udp_socket.h>
#include <iostream>

void udp_echo_server(uint16_t port)
{
    using namespace fb;

    try {
        udp_socket socket(socket_address(port));
        std::cout << "UDP echo server listening on port " << port << std::endl;

        char buffer[1024];

        while (true) {
            socket_address sender;
            int bytes = socket.receive_from(buffer, sizeof(buffer), sender);

            if (bytes > 0) {
                std::cout << "Received " << bytes << " bytes from "
                         << sender.to_string() << std::endl;

                // Echo back
                socket.send_to(buffer, bytes, sender);
            }
        }

    } catch (const std::system_error& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

int main()
{
    fb::initialize_library();
    udp_echo_server(5000);
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: UDP Client with Timeout

```cpp
#include <fb/udp_socket.h>
#include <iostream>

std::string udp_request_response(const socket_address& server,
                                 const std::string& request,
                                 std::chrono::milliseconds timeout)
{
    using namespace fb;

    udp_socket socket;

    // Send request
    socket.send_to(request, server);

    // Wait for response with timeout
    if (socket.poll_read(timeout)) {
        std::string response;
        socket_address from;
        socket.receive_from(response, 1024, from);

        // Verify response is from server
        if (from == server) {
            return response;
        }
    }

    throw `std::system_error` (std::errc::timed_out)("No response from server");
}

int main()
{
    fb::initialize_library();

    try {
        auto response = udp_request_response(
            fb::socket_address("192.168.1.1", 5000),
            "PING",
            std::chrono::seconds(1)
        );

        std::cout << "Response: " << response << std::endl;

    } catch (const fb::`std::system_error` (std::errc::timed_out)& e) {
        std::cerr << "Timeout: " << e.what() << std::endl;
    }

    fb::cleanup_library();
    return 0;
}
```

---

### Example 3: Broadcast Discovery

```cpp
#include <fb/udp_socket.h>
#include <iostream>
#include <thread>
#include <vector>

void discover_servers(uint16_t port)
{
    using namespace fb;

    try {
        // Create socket for broadcasting
        udp_socket socket;
        socket.set_broadcast(true);

        // Send discovery broadcast
        std::cout << "Broadcasting discovery..." << std::endl;
        socket.send_to("DISCOVER", 8, socket_address("255.255.255.255", port));

        // Listen for responses
        std::vector<socket_address> servers;
        auto start = std::chrono::steady_clock::now();

        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            if (socket.poll_read(std::chrono::milliseconds(100))) {
                std::string response;
                socket_address server;
                socket.receive_from(response, 1024, server);

                if (response == "AVAILABLE") {
                    servers.push_back(server);
                    std::cout << "Found server: " << server.to_string() << std::endl;
                }
            }
        }

        std::cout << "Discovered " << servers.size() << " servers" << std::endl;

    } catch (const std::system_error& e) {
        std::cerr << "Discovery error: " << e.what() << std::endl;
    }
}
```

---

### Example 4: Multicast Group

```cpp
#include <fb/udp_socket.h>
#include <iostream>
#include <thread>

// Multicast sender
void multicast_sender(const std::string& group, uint16_t port)
{
    using namespace fb;

    udp_socket socket;
    socket.set_multicast_ttl(1);  // Local subnet only

    socket_address multicast_addr(group, port);

    for (int i = 0; i < 10; ++i) {
        std::string message = "Message " + std::to_string(i);
        socket.send_to(message, multicast_addr);
        std::cout << "Sent: " << message << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Multicast receiver
void multicast_receiver(const std::string& group, uint16_t port)
{
    using namespace fb;

    udp_socket socket(socket_address(port));

    // Join multicast group
    socket.join_group(socket_address(group, port));
    std::cout << "Joined multicast group " << group << std::endl;

    for (int i = 0; i < 10; ++i) {
        std::string message;
        socket_address sender;
        socket.receive_from(message, 1024, sender);

        std::cout << "Received: " << message
                  << " from " << sender.to_string() << std::endl;
    }

    socket.leave_group(socket_address(group, port));
}

int main()
{
    fb::initialize_library();

    std::thread sender(multicast_sender, "239.255.0.1", 5000);
    std::thread receiver(multicast_receiver, "239.255.0.1", 5000);

    sender.join();
    receiver.join();

    fb::cleanup_library();
    return 0;
}
```

---

### Example 5: Round-Trip Time Measurement

```cpp
#include <fb/udp_socket.h>
#include <iostream>
#include <chrono>

void measure_rtt(const socket_address& server, int count)
{
    using namespace fb;

    udp_socket socket;
    double total_rtt = 0;
    int successful = 0;

    for (int i = 0; i < count; ++i) {
        auto start = std::chrono::high_resolution_clock::now();

        // Send ping
        socket.send_to("PING", 4, server);

        // Wait for pong
        if (socket.poll_read(std::chrono::seconds(1))) {
            char buffer[1024];
            socket_address from;
            int bytes = socket.receive_from(buffer, sizeof(buffer), from);

            auto end = std::chrono::high_resolution_clock::now();
            auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            if (bytes > 0 && from == server) {
                double rtt_ms = rtt.count() / 1000.0;
                total_rtt += rtt_ms;
                successful++;

                std::cout << "Reply from " << server.to_string()
                         << ": time=" << rtt_ms << "ms" << std::endl;
            }
        } else {
            std::cout << "Request timeout" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (successful > 0) {
        std::cout << "\nAverage RTT: " << (total_rtt / successful) << "ms" << std::endl;
        std::cout << "Success rate: " << (successful * 100 / count) << "%" << std::endl;
    }
}
```

---

## Common Patterns

### Pattern 1: Request-Response with Retry

```cpp
std::optional<std::string> udp_request_with_retry(
    fb::udp_socket& socket,
    const fb::socket_address& server,
    const std::string& request,
    int max_attempts = 3)
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        socket.send_to(request, server);

        if (socket.poll_read(std::chrono::seconds(1))) {
            std::string response;
            fb::socket_address from;
            socket.receive_from(response, 1024, from);

            if (from == server) {
                return response;
            }
        }

        std::cout << "Attempt " << attempt << " failed, retrying..." << std::endl;
    }

    return std::nullopt;  // All attempts failed
}
```

---

### Pattern 2: Datagram Size Checking

```cpp
void safe_send(fb::udp_socket& socket,
              const std::vector<char>& data,
              const fb::socket_address& dest)
{
    const int MAX_SAFE_SIZE = 1400;  // Safe for most networks

    if (data.size() > MAX_SAFE_SIZE) {
        // Fragment or use TCP instead
        throw std::runtime_error("Datagram too large");
    }

    socket.send_to(data.data(), data.size(), dest);
}
```

---

### Pattern 3: Non-Blocking Receive Loop

```cpp
void process_datagrams(fb::udp_socket& socket)
{
    while (running) {
        if (socket.poll_read(std::chrono::milliseconds(100))) {
            char buffer[1024];
            fb::socket_address sender;
            int bytes = socket.receive_from(buffer, sizeof(buffer), sender);

            if (bytes > 0) {
                process_datagram(buffer, bytes, sender);
            }
        }

        // Can do other work here
        perform_maintenance();
    }
}
```

---

## Performance Considerations

### Datagram Size

```cpp
// Good: Keep datagrams small to avoid fragmentation
const int RECOMMENDED_SIZE = 1400;  // Below typical MTU of 1500

// Bad: Large datagrams cause fragmentation and packet loss
const int TOO_LARGE = 60000;  // Will fragment
```

### Buffer Sizing

```cpp
// Receive buffer should match expected datagram size
char buffer[1500];  // Good for most UDP applications

// Set OS buffer size for high-throughput applications
socket.set_receive_buffer_size(256 * 1024);
```

### Multicast Efficiency

```cpp
// Limit multicast scope
socket.set_multicast_ttl(1);  // Local subnet only

// Disable loopback if not needed
socket.set_multicast_loopback(false);
```

---

## Error Handling

### Datagram Loss

UDP does not guarantee delivery. Handle missing data gracefully:

```cpp
// Use sequence numbers
struct Datagram {
    uint32_t sequence;
    char data[1024];
};

// Detect gaps
uint32_t last_sequence = 0;
Datagram dgram;

while (socket.receive_from(&dgram, sizeof(dgram), sender) > 0) {
    if (dgram.sequence != last_sequence + 1) {
        std::cout << "Lost " << (dgram.sequence - last_sequence - 1)
                  << " datagrams" << std::endl;
    }
    last_sequence = dgram.sequence;
}
```

---

## Thread Safety

udp_socket is **not thread-safe** for concurrent operations on the same socket:

```cpp
// BAD: Multiple threads using same socket
fb::udp_socket socket;
std::thread t1([&]() { socket.send_to("data1", ...); });  // Race!
std::thread t2([&]() { socket.send_to("data2", ...); });  // Race!

// GOOD: Each thread has its own socket
std::thread t1([]() {
    fb::udp_socket socket1;
    socket1.send_to("data1", ...);
});
```

---

## Complete API Reference

| Category | Member | Description |
|----------|--------|-------------|
| **Constructors** | `udp_socket()` | Default constructor (IPv4) |
| | `udp_socket(Family)` | Specify address family |
| | `udp_socket(address)` | Create and bind |
| | `udp_socket(socket_t)` | From descriptor |
| **Datagram I/O** | `send_to(buffer, len, addr)` | Send datagram |
| | `send_to(string, addr)` | Send string datagram |
| | `receive_from(buffer, len, addr)` | Receive datagram |
| | `receive_from(string, len, addr)` | Receive into string |
| **Connected Mode** | `connect(address)` | Associate with peer |
| | `send(message)` | Send to connected peer |
| | `receive(buffer, len)` | Receive from peer |
| | `is_connected()` | Check if connected |
| **Broadcast** | `set_broadcast(bool)` | Enable/disable broadcast |
| | `get_broadcast()` | Get broadcast setting |
| **Multicast** | `join_group(group)` | Join multicast group |
| | `join_group(group, iface)` | Join on interface |
| | `leave_group(group)` | Leave multicast group |
| | `set_multicast_ttl(ttl)` | Set multicast TTL |
| | `get_multicast_ttl()` | Get multicast TTL |
| | `set_multicast_loopback(bool)` | Control loopback |
| | `get_multicast_loopback()` | Get loopback setting |
| **Polling** | `poll_read(timeout)` | Check read readiness |
| | `poll_write(timeout)` | Check write readiness |
| **Properties** | `max_datagram_size()` | Get max datagram size |

---

## See Also

- [`tcp_client.md`](tcp_client.md) - TCP client sockets
- [`udp_server.md`](udp_server.md) - Multi-threaded UDP server
- [`socket_base.md`](socket_base.md) - Base socket functionality
- [`poll_set.md`](poll_set.md) - Multi-socket polling
- [`examples.md`](examples.md) - Practical usage examples

---

*udp_socket - Fast, connectionless datagram communication*
