# fb::tcp_server_connection - TCP Connection Handler Base Class

## Overview

The [`fb::tcp_server_connection`](../include/fb/tcp_server_connection.h) class is the base class for handling individual TCP client connections in a multi-threaded server. Each connection runs in its own thread and manages the complete lifecycle of a single client session.

**Key Features:**
- Base class for custom connection handlers
- Runs in dedicated thread per connection
- Manages single client connection lifecycle
- Access to client socket and address
- Stop/timeout mechanisms
- Signal-based events
- Exception-safe design

**Namespace:** `fb`

**Header:** `#include <fb/tcp_server_connection.h>`

---

## Quick Start

```cpp
#include <fb/tcp_server_connection.h>
#include <iostream>

using namespace fb;

// Define custom connection handler
class EchoConnection : public tcp_server_connection
{
public:
    EchoConnection(tcp_client socket, const socket_address& client_addr)
        : tcp_server_connection(std::move(socket), client_addr)
    {}

    void run() override
    {
        std::cout << "Client connected: " << client_address().to_string() << std::endl;

        try {
            std::string buffer;

            while (!stop_requested()) {
                int bytes = socket().receive(buffer, 1024);

                if (bytes <= 0) {
                    break;  // Connection closed
                }

                // Echo back
                socket().send(buffer);
            }

        } catch (const std::system_error& e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
        }

        std::cout << "Client disconnected" << std::endl;
    }
};

// Use with tcp_server
auto factory = [](tcp_client socket, const socket_address& addr) {
    return std::make_unique<EchoConnection>(std::move(socket), addr);
};
```

---

## Construction

### Constructor

Creates a connection handler for a client socket.

```cpp
tcp_server_connection(tcp_client socket, const socket_address& client_address);
```

**Parameters:**
- `socket` - Connected client socket (moved)
- `client_address` - Client's network address

**Example:**
```cpp
class MyConnection : public tcp_server_connection
{
public:
    MyConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        // Custom initialization
    }
};
```

---

### Move Constructor

```cpp
tcp_server_connection(tcp_server_connection&& other) noexcept;
```

**Note:** Connection handlers are move-only (no copy)

---

## Pure Virtual Method

### run()

Implements the connection handling logic (must override).

```cpp
virtual void run() = 0;
```

**Behavior:**
- Called when connection starts
- Runs in dedicated thread
- Should loop while `!stop_requested()`
- Returns when connection completes

**Example:**
```cpp
void run() override
{
    // Connection initialization
    socket().send("Welcome!");

    // Main processing loop
    while (!stop_requested()) {
        std::string request;
        int bytes = socket().receive(request, 4096);

        if (bytes <= 0) {
            break;  // Client disconnected
        }

        process_request(request);
    }

    // Connection cleanup
    cleanup();
}
```

---

## Lifecycle Management

### start()

Starts the connection (called by tcp_server).

```cpp
void start();
```

**Behavior:**
- Emits `onConnectionStarted` signal
- Typically called by tcp_server internally
- Should not be called manually

---

### stop()

Requests the connection to stop.

```cpp
void stop();
```

**Behavior:**
- Sets stop flag (`stop_requested()` returns true)
- Connection should check flag and exit gracefully
- Non-blocking (doesn't wait for stop)

**Example:**
```cpp
// In run() method
while (!stop_requested()) {
    // Process data
    if (should_disconnect()) {
        break;
    }
}
```

---

### stop_requested()

Checks if stop has been requested.

```cpp
bool stop_requested() const;
```

**Returns:** `true` if stop requested, `false` otherwise

**Usage:**
```cpp
void run() override
{
    while (!stop_requested()) {
        // Check periodically
        if (socket().poll_read(std::chrono::milliseconds(100))) {
            process_data();
        }
    }
}
```

---

## Socket Access

### socket()

Gets the client socket.

```cpp
tcp_client& socket();
const tcp_client& socket() const;
```

**Returns:** Reference to the client socket

**Example:**
```cpp
void run() override
{
    // Send data
    socket().send("Hello, client!");

    // Receive data
    std::string data;
    socket().receive(data, 1024);
}
```

---

## Address Information

### client_address()

Gets the client's network address.

```cpp
const socket_address& client_address() const;
```

**Returns:** Client's socket address

**Example:**
```cpp
void run() override
{
    std::cout << "Handling client: "
              << client_address().to_string() << std::endl;

    // Check if from trusted network
    if (is_trusted(client_address().host())) {
        allow_privileged_operations();
    }
}
```

---

### local_address()

Gets the local endpoint address for this connection.

```cpp
socket_address local_address() const;
```

**Returns:** Local socket address

---

## Connection Status

### is_connected()

Checks if the socket is still connected.

```cpp
bool is_connected() const;
```

**Returns:** `true` if connected, `false` otherwise

**Example:**
```cpp
void run() override
{
    while (is_connected() && !stop_requested()) {
        process_data();
    }
}
```

---

### uptime()

Gets the connection uptime.

```cpp
std::chrono::steady_clock::duration uptime() const;
```

**Returns:** Duration since connection started

**Example:**
```cpp
void run() override
{
    while (!stop_requested()) {
        // Check if connection has been idle too long
        if (uptime() > std::chrono::minutes(5) && no_activity()) {
            break;  // Timeout idle connection
        }

        process_data();
    }
}
```

---

## Socket Configuration

### set_timeout()

Sets socket timeout for operations.

```cpp
void set_timeout(const std::chrono::milliseconds& timeout);
```

**Parameters:**
- `timeout` - Timeout duration

**Example:**
```cpp
class MyConnection : public tcp_server_connection
{
public:
    MyConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        set_timeout(std::chrono::seconds(30));
    }
};
```

---

### set_no_delay()

Controls TCP_NODELAY option.

```cpp
void set_no_delay(bool flag);
```

**Parameters:**
- `flag` - `true` to disable Nagle's algorithm

**Example:**
```cpp
MyConnection(tcp_client socket, const socket_address& addr)
    : tcp_server_connection(std::move(socket), addr)
{
    set_no_delay(true);  // Low latency
}
```

---

### set_keep_alive()

Controls SO_KEEPALIVE option.

```cpp
void set_keep_alive(bool flag);
```

**Parameters:**
- `flag` - `true` to enable keepalive

---

## Signal Events

Connection events are communicated via signals:

```cpp
fb::signal<> onConnectionStarted;           // Connection started
fb::signal<> onConnectionClosing;           // Before close
fb::signal<> onConnectionClosed;            // After close
fb::signal<const std::exception&> onException;  // Exception occurred
```

**Example:**
```cpp
class LoggingConnection : public tcp_server_connection
{
public:
    LoggingConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        onConnectionStarted.connect([this]() {
            log("Connection started");
        });

        onConnectionClosing.connect([this]() {
            log("Connection closing");
        });

        onException.connect([this](const std::exception& e) {
            log("Exception: " + std::string(e.what()));
        });
    }
};
```

---

## Protected Virtual Methods

### handle_exception()

Called when an exception occurs (can override).

```cpp
virtual void handle_exception(const std::exception& ex) noexcept;
```

**Parameters:**
- `ex` - The exception that occurred

**Example:**
```cpp
class MyConnection : public tcp_server_connection
{
protected:
    void handle_exception(const std::exception& ex) noexcept override
    {
        // Custom exception handling
        log_error(client_address().to_string() + ": " + ex.what());

        // Call base implementation
        tcp_server_connection::handle_exception(ex);
    }
};
```

---

### on_connection_closing()

Called before the connection closes (can override).

```cpp
virtual void on_connection_closing() noexcept;
```

**Example:**
```cpp
class MyConnection : public tcp_server_connection
{
protected:
    void on_connection_closing() noexcept override
    {
        // Cleanup before close
        save_session_data();
        update_statistics();
    }
};
```

---

## Complete Examples

### Example 1: Echo Server Connection

```cpp
class EchoConnection : public tcp_server_connection
{
public:
    EchoConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {}

    void run() override
    {
        std::cout << "Echo connection from: "
                  << client_address().to_string() << std::endl;

        try {
            std::string buffer;

            while (!stop_requested()) {
                int bytes = socket().receive(buffer, 1024);

                if (bytes <= 0) {
                    break;
                }

                std::cout << "Echoing " << bytes << " bytes" << std::endl;
                socket().send(buffer);
            }

        } catch (const std::system_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};
```

---

### Example 2: HTTP Connection Handler

```cpp
class HTTPConnection : public tcp_server_connection
{
public:
    HTTPConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        set_timeout(std::chrono::seconds(30));
    }

    void run() override
    {
        try {
            // Read HTTP request
            std::string request;
            socket().receive(request, 4096);

            // Parse request
            auto method = parse_method(request);
            auto path = parse_path(request);

            // Generate response
            std::string response;
            if (method == "GET") {
                response = handle_get(path);
            } else {
                response = http_error(405, "Method Not Allowed");
            }

            // Send response
            socket().send(response);

        } catch (const std::system_error& e) {
            std::cerr << "HTTP error: " << e.what() << std::endl;
        }
    }

private:
    std::string handle_get(const std::string& path)
    {
        // Implementation
        return "HTTP/1.1 200 OK\r\n\r\nHello, World!";
    }

    std::string http_error(int code, const std::string& message)
    {
        return "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n\r\n";
    }
};
```

---

### Example 3: Chat Room Connection

```cpp
class ChatConnection : public tcp_server_connection
{
public:
    ChatConnection(tcp_client socket, const socket_address& addr,
                  ChatRoom* room)
        : tcp_server_connection(std::move(socket), addr)
        , room_(room)
    {}

    void run() override
    {
        // Get username
        socket().send("Enter username: ");
        socket().receive(username_, 256);

        room_->join(this, username_);

        try {
            std::string message;

            while (!stop_requested()) {
                int bytes = socket().receive(message, 1024);

                if (bytes <= 0) {
                    break;
                }

                // Broadcast to room
                room_->broadcast(username_ + ": " + message, this);
            }

        } catch (const std::system_error& e) {
            // Error handling
        }

        room_->leave(this);
    }

    void send_message(const std::string& message)
    {
        try {
            socket().send(message + "\n");
        } catch (...) {
            // Client disconnected
        }
    }

private:
    ChatRoom* room_;
    std::string username_;
};
```

---

### Example 4: Authentication Connection

```cpp
class AuthenticatedConnection : public tcp_server_connection
{
public:
    AuthenticatedConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
        , authenticated_(false)
    {}

    void run() override
    {
        // Require authentication
        if (!authenticate()) {
            socket().send("Authentication failed\n");
            return;
        }

        authenticated_ = true;
        socket().send("Authentication successful\n");

        // Process authenticated session
        handle_session();
    }

private:
    bool authenticate()
    {
        socket().send("Username: ");
        std::string username;
        socket().receive(username, 256);

        socket().send("Password: ");
        std::string password;
        socket().receive(password, 256);

        return validate_credentials(username, password);
    }

    void handle_session()
    {
        while (!stop_requested()) {
            std::string command;
            socket().receive(command, 1024);

            auto response = process_command(command);
            socket().send(response);
        }
    }

    bool authenticated_;
};
```

---

### Example 5: Binary Protocol Connection

```cpp
class BinaryProtocolConnection : public tcp_server_connection
{
public:
    BinaryProtocolConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {
        set_no_delay(true);  // Low latency for binary protocol
    }

    void run() override
    {
        try {
            while (!stop_requested()) {
                // Read message header (4 bytes length)
                uint32_t length;
                socket().receive_bytes_exact(&length, sizeof(length));
                length = ntohl(length);

                // Read message body
                std::vector<char> message(length);
                socket().receive_bytes_exact(message.data(), length);

                // Process message
                auto response = process_message(message);

                // Send response
                uint32_t response_length = htonl(response.size());
                socket().send_bytes_all(&response_length, sizeof(response_length));
                socket().send_bytes_all(response.data(), response.size());
            }

        } catch (const std::system_error& e) {
            std::cerr << "Protocol error: " << e.what() << std::endl;
        }
    }

private:
    std::vector<char> process_message(const std::vector<char>& message)
    {
        // Implementation
        return {};
    }
};
```

---

## Common Patterns

### Pattern 1: Request-Response Loop

```cpp
void run() override
{
    while (!stop_requested()) {
        // Read request
        std::string request;
        if (socket().receive(request, 4096) <= 0) {
            break;
        }

        // Process and respond
        std::string response = process_request(request);
        socket().send(response);
    }
}
```

---

### Pattern 2: Idle Timeout

```cpp
void run() override
{
    auto last_activity = std::chrono::steady_clock::now();
    const auto IDLE_TIMEOUT = std::chrono::minutes(5);

    while (!stop_requested()) {
        if (socket().poll_read(std::chrono::milliseconds(100))) {
            process_data();
            last_activity = std::chrono::steady_clock::now();
        } else {
            // Check idle timeout
            if (std::chrono::steady_clock::now() - last_activity > IDLE_TIMEOUT) {
                socket().send("Timeout due to inactivity\n");
                break;
            }
        }
    }
}
```

---

### Pattern 3: State Machine

```cpp
enum class State { Init, Authenticated, Processing, Closing };

void run() override
{
    State state = State::Init;

    while (!stop_requested() && state != State::Closing) {
        switch (state) {
            case State::Init:
                state = handle_init();
                break;
            case State::Authenticated:
                state = handle_authenticated();
                break;
            case State::Processing:
                state = handle_processing();
                break;
            case State::Closing:
                break;
        }
    }
}
```

---

## Thread Safety

Each connection runs in its own thread. The connection object is **not thread-safe** for concurrent access from multiple threads:

```cpp
// GOOD: Each connection in its own thread (automatic)
tcp_server server(...);
server.start();  // Creates threads automatically

// BAD: Don't access connection from multiple threads
class BadConnection : public tcp_server_connection
{
    void run() override
    {
        std::thread t([this]() {
            socket().send("data");  // ! Race condition!
        });
        socket().send("data");  // ! Race condition!
    }
};
```

---

## Complete API Reference

| Category | Member | Description |
|----------|--------|-------------|
| **Construction** | `tcp_server_connection(socket, addr)` | Create connection handler |
| **Pure Virtual** | `run()` | Implement connection logic |
| **Lifecycle** | `start()` | Start connection |
| | `stop()` | Request stop |
| | `stop_requested()` | Check if stop requested |
| **Socket Access** | `socket()` | Get client socket |
| **Address Info** | `client_address()` | Get client address |
| | `local_address()` | Get local address |
| **Status** | `is_connected()` | Check connection status |
| | `uptime()` | Get connection uptime |
| **Configuration** | `set_timeout(duration)` | Set socket timeout |
| | `set_no_delay(bool)` | Control TCP_NODELAY |
| | `set_keep_alive(bool)` | Control SO_KEEPALIVE |
| **Signals** | `onConnectionStarted` | Connection started |
| | `onConnectionClosing` | Before close |
| | `onConnectionClosed` | After close |
| | `onException` | Exception occurred |
| **Protected** | `handle_exception(ex)` | Handle exceptions |
| | `on_connection_closing()` | Before closing |

---

## See Also

- [`tcp_server.md`](tcp_server.md) - Multi-threaded TCP server
- [`tcp_client.md`](tcp_client.md) - TCP client socket
- [`examples.md`](examples.md) - Practical usage examples

---

*tcp_server_connection - Connection handler base class for multi-threaded servers*
