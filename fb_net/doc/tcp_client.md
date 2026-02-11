# fb::tcp_client - TCP Client Socket

## Overview

The [`fb::tcp_client`](../include/fb/tcp_client.h) class provides a high-level interface for TCP client connections. It inherits from `socket_base` and adds TCP-specific functionality including connection management, reliable data transfer, and TCP-specific socket options.

**Key Features:**
- Connection establishment with timeout support
- Reliable, ordered data transfer
- Blocking and non-blocking connection modes
- TCP-specific options (TCP_NODELAY, SO_KEEPALIVE)
- Graceful and forceful shutdown
- Poll-based I/O readiness checking
- Signal-based event notification
- Exception-safe RAII design

**Namespace:** `fb`

**Header:** `#include <fb/tcp_client.h>`

---

## Quick Start

```cpp
#include <fb/tcp_client.h>
#include <iostream>

using namespace fb;

int main()
{
    try {
        // Create TCP client socket
        tcp_client socket(socket_address::IPv4);

        // Connect with timeout
        socket.connect(socket_address("example.com", 80), std::chrono::seconds(5));

        // Enable TCP_NODELAY for low latency
        socket.set_no_delay(true);

        // Send data
        std::string request = "GET / HTTP/1.1\r\n\r\n";
        socket.send(request);

        // Receive response
        std::string response;
        socket.receive(response, 4096);

        std::cout << "Response: " << response << std::endl;

        // Socket automatically closed when destroyed

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
```

---

## Construction

### Default Constructor

Creates an unconnected IPv4 TCP socket.

```cpp
tcp_client();
```

**Example:**
```cpp
tcp_client socket;
socket.connect(socket_address("192.168.1.1", 8080));
```

---

### Family Constructor

Creates an unconnected TCP socket with specified address family.

```cpp
explicit tcp_client(socket_address::Family family);
```

**Parameters:**
- `family` - Address family (IPv4 or IPv6)

**Example:**
```cpp
// IPv4 socket
tcp_client socket_v4(socket_address::IPv4);

// IPv6 socket
tcp_client socket_v6(socket_address::IPv6);
```

---

### Address Constructor

Creates and connects to the specified address.

```cpp
explicit tcp_client(const socket_address& address);
```

**Parameters:**
- `address` - Server address to connect to

**Throws:** `std::system_error (std::errc::connection_refused)`, `std::system_error (std::errc::timed_out)`, `std::system_error`

**Example:**
```cpp
// Connect immediately during construction
tcp_client socket(socket_address("example.com", 80));
// Socket is connected and ready to use
```

---

### Address with Timeout Constructor

Creates and connects with a specified timeout.

```cpp
tcp_client(const socket_address& address, const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `address` - Server address to connect to
- `timeout` - Connection timeout

**Example:**
```cpp
tcp_client socket(socket_address("example.com", 80), std::chrono::seconds(5));
```

---

### Socket Descriptor Constructor

Creates from an existing socket descriptor.

```cpp
explicit tcp_client(socket_t sockfd);
```

**Parameters:**
- `sockfd` - Existing socket file descriptor

**Use Case:**
- Wrapping sockets from `accept()`
- Interoperability with raw socket APIs

**Example:**
```cpp
server_socket server;
// ... bind and listen ...
tcp_client client = server.accept_connection();
```

---

### Move Constructor

```cpp
tcp_client(tcp_client&& other) noexcept;
```

**Example:**
```cpp
tcp_client create_connected_socket()
{
    tcp_client socket;
    socket.connect(socket_address("example.com", 80));
    return socket;  // Move semantics
}

tcp_client socket = create_connected_socket();
```

---

## Connection Management

### connect()

Establishes a connection to the specified address.

```cpp
void connect(const socket_address& address);
void connect(const socket_address& address, const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `address` - Server address to connect to
- `timeout` - Connection timeout (optional)

**Throws:**
- `std::system_error (std::errc::connection_refused)` - Server refused connection
- `std::system_error (std::errc::timed_out)` - Connection attempt timed out
- `std::system_error (std::errc::host_unreachable)` - DNS resolution failed
- `std::system_error` - Other connection errors

**Example:**
```cpp
tcp_client socket(socket_address::IPv4);

// Connect with default timeout
socket.connect(socket_address("192.168.1.1", 8080));

// Connect with custom timeout
socket.connect(socket_address("example.com", 80), std::chrono::seconds(10));
```

---

### connect_non_blocking()

Initiates a non-blocking connection.

```cpp
void connect_non_blocking(const socket_address& address);
```

**Parameters:**
- `address` - Server address to connect to

**Behavior:**
- Returns immediately
- Use `poll_write()` to check when connection completes
- Socket must be in non-blocking mode

**Example:**
```cpp
tcp_client socket;
socket.set_blocking(false);
socket.connect_non_blocking(socket_address("example.com", 80));

// Wait for connection to complete
if (socket.poll_write(std::chrono::seconds(5))) {
    std::cout << "Connected!" << std::endl;
} else {
    std::cout << "Connection timeout" << std::endl;
}
```

---

## Data Transfer

### send()

Sends a string over the connection.

```cpp
int send(const std::string& message);
```

**Parameters:**
- `message` - String to send

**Returns:** Number of bytes sent

**Throws:**
- `ConnectionResetException` - Connection reset by peer
- `ConnectionAbortedException` - Connection aborted
- `std::system_error` - Send error

**Example:**
```cpp
std::string message = "Hello, Server!";
int bytes_sent = socket.send(message);
std::cout << "Sent " << bytes_sent << " bytes" << std::endl;
```

---

### send_bytes()

Sends raw bytes over the connection.

```cpp
int send_bytes(const void* buffer, int length, int flags = 0);
```

**Parameters:**
- `buffer` - Pointer to data buffer
- `length` - Number of bytes to send
- `flags` - Socket send flags (default: 0)

**Returns:** Number of bytes actually sent (may be less than `length`)

**Example:**
```cpp
char data[1024] = {...};
int sent = socket.send_bytes(data, sizeof(data));

if (sent < sizeof(data)) {
    // Partial send, handle remaining data
}
```

---

### send_bytes_all()

Sends all bytes, handling partial sends automatically.

```cpp
int send_bytes_all(const void* buffer, int length, int flags = 0);
```

**Parameters:**
- `buffer` - Pointer to data buffer
- `length` - Number of bytes to send
- `flags` - Socket send flags

**Returns:** Total number of bytes sent (equals `length` on success)

**Behavior:**
- Loops until all data is sent
- Handles partial sends internally
- Blocks until complete or error

**Example:**
```cpp
char data[1024] = {...};
int sent = socket.send_bytes_all(data, sizeof(data));
assert(sent == sizeof(data));  // All data sent
```

---

### receive()

Receives data into a string.

```cpp
int receive(std::string& buffer, int length);
```

**Parameters:**
- `buffer` - String to receive data into (cleared first)
- `length` - Maximum bytes to receive

**Returns:** Number of bytes received (0 = connection closed)

**Example:**
```cpp
std::string response;
int bytes = socket.receive(response, 4096);

if (bytes > 0) {
    std::cout << "Received: " << response << std::endl;
} else if (bytes == 0) {
    std::cout << "Connection closed by peer" << std::endl;
}
```

---

### receive_bytes()

Receives raw bytes into a buffer.

```cpp
int receive_bytes(void* buffer, int length, int flags = 0);
```

**Parameters:**
- `buffer` - Pointer to receive buffer
- `length` - Maximum bytes to receive
- `flags` - Socket receive flags

**Returns:** Number of bytes received (0 = connection closed, -1 = error)

**Example:**
```cpp
char buffer[1024];
int bytes = socket.receive_bytes(buffer, sizeof(buffer));

if (bytes > 0) {
    // Process buffer[0] to buffer[bytes-1]
}
```

---

### receive_bytes_exact()

Receives exactly the specified number of bytes.

```cpp
int receive_bytes_exact(void* buffer, int length, int flags = 0);
```

**Parameters:**
- `buffer` - Pointer to receive buffer
- `length` - Exact number of bytes to receive
- `flags` - Socket receive flags

**Returns:** Number of bytes received (equals `length` on success)

**Behavior:**
- Loops until all bytes received
- Handles partial receives internally
- Blocks until complete or connection closed

**Example:**
```cpp
// Receive message header (4 bytes)
uint32_t msg_length;
socket.receive_bytes_exact(&msg_length, sizeof(msg_length));

// Receive message body
std::vector<char> message(msg_length);
socket.receive_bytes_exact(message.data(), msg_length);
```

---

## Connection Shutdown

### shutdown()

Performs graceful shutdown of both send and receive.

```cpp
void shutdown();
```

**Behavior:**
- Closes both send and receive channels
- TCP FIN sent to peer
- Waits for peer acknowledgment

**Example:**
```cpp
socket.send("Goodbye!");
socket.shutdown();  // Graceful close
```

---

### shutdown_send()

Shuts down the sending side only (half-close).

```cpp
int shutdown_send();
```

**Returns:** 0 on success, -1 on error

**Use Case:**
- Signal "no more data to send"
- Continue receiving data from peer

**Example:**
```cpp
socket.send("Final message");
socket.shutdown_send();

// Can still receive data from peer
std::string response;
while (socket.receive(response, 1024) > 0) {
    // Process response
}
```

---

### shutdown_receive()

Shuts down the receiving side only.

```cpp
void shutdown_receive();
```

**Use Case:**
- Stop receiving data
- Continue sending data

---

### send_urgent()

Sends urgent (out-of-band) data.

```cpp
void send_urgent(unsigned char data);
```

**Parameters:**
- `data` - Single byte of urgent data

**Use Case:**
- Send interrupt signals
- Priority data

---

## TCP-Specific Options

### set_no_delay() / get_no_delay()

Controls Nagle's algorithm (TCP_NODELAY).

```cpp
void set_no_delay(bool flag);
bool get_no_delay();
```

**Parameters:**
- `flag` - `true` to disable Nagle's algorithm, `false` to enable

**When to Use:**
- **Enable (set to true)**: Low-latency applications, gaming, real-time data
- **Disable (set to false)**: Bulk data transfer where latency is less critical

**Example:**
```cpp
// Disable Nagle's algorithm for low latency
socket.set_no_delay(true);

// Check current setting
bool no_delay = socket.get_no_delay();
```

---

### set_keep_alive() / get_keep_alive()

Controls TCP keepalive mechanism.

```cpp
void set_keep_alive(bool flag);
bool get_keep_alive();
```

**Parameters:**
- `flag` - `true` to enable keepalive, `false` to disable

**When to Use:**
- Long-lived connections
- Detect dead connections
- Keep NAT mappings alive

**Example:**
```cpp
// Enable keepalive for long-lived connection
socket.set_keep_alive(true);
```

---

## I/O Readiness Polling

### poll_read()

Checks if data is available for reading.

```cpp
bool poll_read(const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `timeout` - Maximum time to wait

**Returns:** `true` if data available, `false` if timeout

**Example:**
```cpp
if (socket.poll_read(std::chrono::milliseconds(100))) {
    std::string data;
    socket.receive(data, 1024);
}
```

---

### poll_write()

Checks if socket is ready for writing.

```cpp
bool poll_write(const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `timeout` - Maximum time to wait

**Returns:** `true` if ready to write, `false` if timeout

**Example:**
```cpp
if (socket.poll_write(std::chrono::milliseconds(100))) {
    socket.send("Data to send");
}
```

---

## Signal-Based Events

tcp_client provides signals for event-driven programming:

```cpp
fb::signal<const socket_address&> onConnected;
fb::signal<> onDisconnected;
fb::signal<const std::string&> onConnectionError;
fb::signal<const void*, size_t> onDataReceived;
fb::signal<size_t> onDataSent;
fb::signal<const std::string&> onSendError;
fb::signal<const std::string&> onReceiveError;
fb::signal<> onShutdownInitiated;
```

**Example:**
```cpp
tcp_client socket;

// Connect signal handlers
socket.onConnected.connect([](const socket_address& addr) {
    std::cout << "Connected to " << addr.to_string() << std::endl;
});

socket.onDataReceived.connect([](const void* data, size_t len) {
    std::cout << "Received " << len << " bytes" << std::endl;
});

socket.onDisconnected.connect([]() {
    std::cout << "Disconnected" << std::endl;
});

// Perform operations - signals fire automatically
socket.connect(socket_address("example.com", 80));
socket.send("Hello");
```

---

## Complete Examples

### Example 1: Simple HTTP GET Request

```cpp
#include <fb/tcp_client.h>
#include <iostream>

void http_get(const std::string& host, const std::string& path)
{
    using namespace fb;

    try {
        tcp_client socket(socket_address::IPv4);

        // Connect with timeout
        socket.connect(socket_address(host, 80), std::chrono::seconds(10));

        // Build request
        std::string request =
            "GET " + path + " HTTP/1.1\r\n"
            "Host: " + host + "\r\n"
            "Connection: close\r\n"
            "\r\n";

        // Send request
        socket.send(request);

        // Receive response
        std::string response;
        while (socket.receive(response, 4096) > 0) {
            std::cout << response;
        }

    } catch (const std::system_error (std::errc::connection_refused)& e) {
        std::cerr << "Connection refused: " << e.what() << std::endl;
    } catch (const std::system_error (std::errc::timed_out)& e) {
        std::cerr << "Timeout: " << e.what() << std::endl;
    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main()
{
    fb::initialize_library();
    http_get("example.com", "/");
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Binary Protocol with Length Prefix

```cpp
#include <fb/tcp_client.h>
#include <vector>
#include <cstdint>

void send_message(fb::tcp_client& socket, const std::vector<char>& message)
{
    // Send length prefix (4 bytes, network byte order)
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    socket.send_bytes_all(&length, sizeof(length));

    // Send message data
    socket.send_bytes_all(message.data(), message.size());
}

std::vector<char> receive_message(fb::tcp_client& socket)
{
    // Receive length prefix
    uint32_t length;
    socket.receive_bytes_exact(&length, sizeof(length));
    length = ntohl(length);

    // Receive message data
    std::vector<char> message(length);
    socket.receive_bytes_exact(message.data(), length);

    return message;
}
```

---

### Example 3: Connection with Retry Logic

```cpp
#include <fb/tcp_client.h>
#include <thread>

bool connect_with_retry(fb::tcp_client& socket,
                       const fb::socket_address& address,
                       int max_attempts = 3)
{
    using namespace fb;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            std::cout << "Connection attempt " << attempt << "..." << std::endl;

            socket.connect(address, std::chrono::seconds(5));

            std::cout << "Connected!" << std::endl;
            return true;

        } catch (const std::system_error (std::errc::connection_refused)& e) {
            std::cerr << "Connection refused: " << e.what() << std::endl;

            if (attempt < max_attempts) {
                int delay = attempt * 2;  // Exponential backoff
                std::cout << "Retrying in " << delay << " seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(delay));
            }

        } catch (const std::system_error (std::errc::timed_out)& e) {
            std::cerr << "Timeout: " << e.what() << std::endl;

            if (attempt == max_attempts) {
                return false;
            }
        }
    }

    return false;
}
```

---

### Example 4: Non-Blocking Connection

```cpp
#include <fb/tcp_client.h>
#include <iostream>

void non_blocking_connect_example()
{
    using namespace fb;

    tcp_client socket(socket_address::IPv4);

    // Set non-blocking mode
    socket.set_blocking(false);

    try {
        // Initiate connection
        socket.connect_non_blocking(socket_address("example.com", 80));

        std::cout << "Connection initiated..." << std::endl;

        // Poll for connection completion
        if (socket.poll_write(std::chrono::seconds(10))) {
            std::cout << "Connected!" << std::endl;

            // Switch back to blocking mode for easier I/O
            socket.set_blocking(true);

            // Use the connection
            socket.send("GET / HTTP/1.1\r\n\r\n");

        } else {
            std::cout << "Connection timeout" << std::endl;
        }

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

---

### Example 5: Low-Latency Configuration

```cpp
#include <fb/tcp_client.h>

void configure_low_latency(fb::tcp_client& socket)
{
    // Disable Nagle's algorithm for immediate send
    socket.set_no_delay(true);

    // Set small buffer sizes to reduce latency
    socket.set_send_buffer_size(4096);
    socket.set_receive_buffer_size(4096);

    // Set timeouts
    socket.set_send_timeout(std::chrono::milliseconds(100));
    socket.set_receive_timeout(std::chrono::milliseconds(100));
}
```

---

## Common Patterns

### Pattern 1: Request-Response

```cpp
std::string request_response(fb::tcp_client& socket,
                            const std::string& request)
{
    // Send request
    socket.send(request);

    // Receive response
    std::string response;
    socket.receive(response, 4096);

    return response;
}
```

---

### Pattern 2: Streaming Data

```cpp
void stream_data(fb::tcp_client& socket, std::istream& input)
{
    char buffer[4096];

    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0) {
        socket.send_bytes_all(buffer, input.gcount());
    }
}
```

---

### Pattern 3: Connection Pool

```cpp
class ConnectionPool
{
public:
    fb::tcp_client acquire(const fb::socket_address& address)
    {
        // Try to reuse existing connection
        for (auto& conn : connections_) {
            if (!conn.in_use) {
                conn.in_use = true;
                return std::move(conn.socket);
            }
        }

        // Create new connection
        fb::tcp_client socket;
        socket.connect(address);
        return socket;
    }

    void release(fb::tcp_client socket)
    {
        connections_.push_back({std::move(socket), false});
    }

private:
    struct Connection {
        fb::tcp_client socket;
        bool in_use;
    };

    std::vector<Connection> connections_;
};
```

---

## Performance Considerations

### TCP_NODELAY

```cpp
// For latency-sensitive applications
socket.set_no_delay(true);  // Disable Nagle's algorithm

// For throughput-sensitive applications
socket.set_no_delay(false);  // Enable Nagle's algorithm (default)
```

### Buffer Sizes

```cpp
// Increase for high-throughput applications
socket.set_send_buffer_size(64 * 1024);
socket.set_receive_buffer_size(64 * 1024);

// Decrease for low-latency applications
socket.set_send_buffer_size(4 * 1024);
socket.set_receive_buffer_size(4 * 1024);
```

### Timeout Configuration

```cpp
// Aggressive timeouts for interactive applications
socket.set_receive_timeout(std::chrono::milliseconds(100));

// Longer timeouts for bulk transfers
socket.set_receive_timeout(std::chrono::seconds(30));
```

---

## Error Handling

### Common Exceptions

| Exception | When Thrown | Recommended Action |
|-----------|-------------|-------------------|
| `std::system_error (std::errc::connection_refused)` | Server not listening | Verify server, retry later |
| `std::system_error (std::errc::timed_out)` | Operation timed out | Increase timeout, retry |
| `ConnectionResetException` | Peer reset connection | Reconnect if appropriate |
| `ConnectionAbortedException` | Connection aborted | Check network, reconnect |
| `std::system_error (std::errc::host_unreachable)` | DNS resolution failed | Check hostname, DNS settings |

### Exception Handling Example

```cpp
try {
    socket.connect(address, std::chrono::seconds(5));
    socket.send(data);
    socket.receive(response, 4096);

} catch (const std::system_error (std::errc::connection_refused)& e) {
    // Server not available
    log_error("Server unavailable: " + address.to_string());
    return try_backup_server();

} catch (const std::system_error (std::errc::timed_out)& e) {
    // Operation timed out
    log_warning("Timeout connecting to " + address.to_string());
    return retry_with_longer_timeout();

} catch (const ConnectionResetException& e) {
    // Connection lost
    log_error("Connection reset");
    return reconnect();

} catch (const std::system_error& e) {
    // Other network error
    log_error("Network error: " + std::string(e.what()));
    return handle_error();
}
```

---

## Thread Safety

tcp_client is **not thread-safe** for concurrent operations on the same socket:

```cpp
// BAD: Multiple threads using same socket
tcp_client socket;
std::thread t1([&]() { socket.send("data1"); });  // Race condition!
std::thread t2([&]() { socket.send("data2"); });  // Race condition!

// GOOD: Each thread has its own socket
std::thread t1([]() {
    tcp_client socket1;
    socket1.connect(...);
    socket1.send("data1");
});

std::thread t2([]() {
    tcp_client socket2;
    socket2.connect(...);
    socket2.send("data2");
});
```

---

## Complete API Reference

| Category | Member | Description |
|----------|--------|-------------|
| **Constructors** | `tcp_client()` | Default constructor (IPv4) |
| | `tcp_client(Family)` | Specify address family |
| | `tcp_client(socket_address)` | Create and connect |
| | `tcp_client(socket_address, timeout)` | Create and connect with timeout |
| | `tcp_client(socket_t)` | From existing socket descriptor |
| **Connection** | `connect(address)` | Connect to server |
| | `connect(address, timeout)` | Connect with timeout |
| | `connect_non_blocking(address)` | Non-blocking connect |
| **Data Transfer** | `send(string)` | Send string data |
| | `send_bytes(buffer, len)` | Send raw bytes |
| | `send_bytes_all(buffer, len)` | Send all bytes |
| | `receive(string, len)` | Receive into string |
| | `receive_bytes(buffer, len)` | Receive raw bytes |
| | `receive_bytes_exact(buffer, len)` | Receive exact amount |
| **Shutdown** | `shutdown()` | Graceful shutdown (both) |
| | `shutdown_send()` | Shutdown sending |
| | `shutdown_receive()` | Shutdown receiving |
| | `send_urgent(byte)` | Send urgent data |
| **TCP Options** | `set_no_delay(bool)` | Control Nagle's algorithm |
| | `get_no_delay()` | Get TCP_NODELAY setting |
| | `set_keep_alive(bool)` | Control keepalive |
| | `get_keep_alive()` | Get keepalive setting |
| **Polling** | `poll_read(timeout)` | Check read readiness |
| | `poll_write(timeout)` | Check write readiness |
| **Signals** | `onConnected` | Connection established |
| | `onDisconnected` | Connection closed |
| | `onDataReceived` | Data received |
| | `onDataSent` | Data sent |
| | Various error signals | Error notifications |

---

## See Also

- [`socket_base.md`](socket_base.md) - Base socket functionality
- [`socket_address.md`](socket_address.md) - Network addressing
- [`server_socket.md`](server_socket.md) - Server-side TCP sockets
- [`exceptions.md`](exceptions.md) - Exception handling
- [`examples.md`](examples.md) - Practical usage examples

---

*tcp_client - Reliable TCP client connections with modern C++*
