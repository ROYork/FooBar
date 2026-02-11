# socket_stream - Standard C++ Stream Interface for TCP Sockets

## Table of Contents
- [Overview](#overview)
- [Quick Start](#quick-start)
- [socket_stream_buf](#socket_stream_buf)
  - [Construction](#construction-buf)
  - [Methods](#methods-buf)
- [socket_stream](#socket_stream-1)
  - [Construction](#construction-stream)
  - [Stream Operations](#stream-operations)
  - [Configuration Methods](#configuration-methods)
  - [Status Queries](#status-queries)
- [Stream-Based I/O](#stream-based-io)
- [Complete Examples](#complete-examples)
- [Common Patterns](#common-patterns)
- [Performance Considerations](#performance-considerations)
- [Error Handling](#error-handling)
- [See Also](#see-also)

---

## Overview

The `socket_stream` class provides a standard C++ `std::iostream` compatible interface for TCP socket communication. It wraps `tcp_client` to enable formatted input/output operations using familiar stream operators (`<<`, `>>`), `getline()`, and other iostream methods.

**Key Features:**
- **std::iostream Compatibility** - Use standard stream operators and methods
- **Formatted I/O** - Automatic type conversion and formatting
- **Dual Buffering** - Separate input and output buffers for efficiency
- **Transparent Connection** - Create stream directly from address or existing socket
- **TCP Options** - TCP_NODELAY, SO_KEEPALIVE, timeouts
- **Exception Safety** - Proper error handling with stream states
- **Lazy Buffer Initialization** - Buffers created on first use for efficiency

**Use Cases:**
- Interactive TCP clients with formatted input/output
- Text-based protocols (HTTP, SMTP, telnet)
- Configuration over network
- Remote command execution
- Protocol debugging with readable text format

**Include:** `<fb/socket_stream.h>`
**Namespace:** `fb`
**Platform Support:** Windows, macOS, Linux, BSD

---

## Quick Start

### Basic HTTP Client

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    // Create stream to web server
    socket_stream stream(socket_address("example.com", 80));

    // Send HTTP request using stream operators
    stream << "GET / HTTP/1.1\r\n"
           << "Host: example.com\r\n"
           << "Connection: close\r\n"
           << "\r\n";

    // Read response line by line
    std::string line;
    while (std::getline(stream, line)) {
        std::cout << line << "\n";
    }

    stream.close();
    fb::cleanup_library();
    return 0;
}
```

### Interactive Chat Client

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    socket_stream stream(socket_address("localhost", 8888));

    std::string message;
    while (std::cout << "> " && std::getline(std::cin, message)) {
        stream << message << "\n";
        stream.flush();

        std::string response;
        if (std::getline(stream, response)) {
            std::cout << "Server: " << response << "\n";
        }
    }

    stream.close();
    fb::cleanup_library();
    return 0;
}
```

---

## socket_stream_buf

The `socket_stream_buf` class is the underlying stream buffer that implements `std::streambuf` and manages buffering for socket I/O.

### Construction

#### Constructor

```cpp
explicit socket_stream_buf(tcp_client& socket, std::size_t buffer_size = 8192);
```

Creates a stream buffer for the given TCP socket.

**Parameters:**
- `socket` - Reference to an existing tcp_client (must remain valid)
- `buffer_size` - Size of internal buffers (default: 8192 bytes)

**Throws:** ``std::system_error`` if buffer_size is zero

**Example:**
```cpp
tcp_client client;
client.connect(socket_address("127.0.0.1", 8888));

socket_stream_buf buf(client, 16384);  // 16KB buffers
```

---

### Methods

#### socket()

```cpp
tcp_client& socket();
```

Accesses the underlying TCP socket.

**Returns:** Reference to tcp_client

**Example:**
```cpp
socket_stream_buf buf(client);
std::cout << "Connected to: " << buf.socket().remote_address().to_string() << "\n";
```

---

#### buffer_size()

```cpp
std::size_t buffer_size() const;
```

Gets the current buffer size.

**Returns:** Buffer size in bytes

**Example:**
```cpp
size_t size = buf.buffer_size();
std::cout << "Buffer size: " << size << " bytes\n";
```

---

#### set_buffer_size()

```cpp
void set_buffer_size(std::size_t size);
```

Changes the buffer size. Buffers must be empty.

**Parameters:**
- `size` - New buffer size (must be > 0)

**Throws:** ``std::system_error`` if buffers contain data or size is zero

**Example:**
```cpp
buf.set_buffer_size(32768);  // Resize to 32KB
```

---

## socket_stream

The `socket_stream` class is a full `std::iostream` implementation for TCP sockets.

### Construction

#### Existing Socket Constructor

```cpp
explicit socket_stream(tcp_client& socket, std::size_t buffer_size = 8192);
```

Wraps an existing TCP socket.

**Parameters:**
- `socket` - Reference to existing tcp_client (must remain valid)
- `buffer_size` - Stream buffer size (default: 8192)

**Example:**
```cpp
tcp_client client;
client.connect(socket_address("127.0.0.1", 8888));

socket_stream stream(client);  // Wrap existing socket
```

---

#### Address Constructor

```cpp
explicit socket_stream(const socket_address& address, std::size_t buffer_size = 8192);
```

Creates and connects to a remote address.

**Parameters:**
- `address` - Remote endpoint
- `buffer_size` - Stream buffer size (default: 8192)

**Throws:** `std::system_error` if connection fails

**Example:**
```cpp
socket_stream stream(socket_address("example.com", 80));
```

---

#### Address with Timeout Constructor

```cpp
socket_stream(const socket_address& address,
             const std::chrono::milliseconds& timeout,
             std::size_t buffer_size = 8192);
```

Creates, connects with timeout, and wraps socket.

**Parameters:**
- `address` - Remote endpoint
- `timeout` - Connection timeout
- `buffer_size` - Stream buffer size (default: 8192)

**Example:**
```cpp
socket_stream stream(socket_address("example.com", 80),
                     std::chrono::seconds(5));  // 5 second timeout
```

---

### Stream Operations

#### std::cout Style Output

```cpp
stream << "Hello, Server!\n";
stream << "Number: " << 42 << "\n";
stream << "Float: " << 3.14 << "\n";
```

All standard output operators work exactly like `std::cout`.

---

#### std::cin Style Input

```cpp
std::string line;
std::getline(stream, line);  // Read line

int number;
stream >> number;  // Read formatted data
```

All standard input operators work exactly like `std::cin`.

---

#### flush()

```cpp
stream.flush();
```

Flushes both input and output buffers.

**Example:**
```cpp
stream << "Request\n";
stream.flush();  // Ensure data is sent
```

---

### Configuration Methods

#### set_no_delay()

```cpp
void set_no_delay(bool flag);
```

Enables/disables TCP_NODELAY (Nagle's algorithm).

**Parameters:**
- `flag` - true to disable Nagle (low-latency), false to enable

**Example:**
```cpp
stream.set_no_delay(true);  // Disable Nagle for interactive use
```

---

#### get_no_delay()

```cpp
bool get_no_delay();
```

Gets the TCP_NODELAY setting.

**Returns:** true if Nagle disabled, false otherwise

---

#### set_keep_alive()

```cpp
void set_keep_alive(bool flag);
```

Enables/disables SO_KEEPALIVE for connection monitoring.

**Parameters:**
- `flag` - true to enable, false to disable

**Example:**
```cpp
stream.set_keep_alive(true);  // Enable keepalive
```

---

#### get_keep_alive()

```cpp
bool get_keep_alive();
```

Gets the SO_KEEPALIVE setting.

**Returns:** true if keepalive enabled, false otherwise

---

#### set_send_timeout()

```cpp
void set_send_timeout(const std::chrono::milliseconds& timeout);
```

Sets timeout for send operations.

**Parameters:**
- `timeout` - Send timeout (0 = blocking)

**Example:**
```cpp
stream.set_send_timeout(std::chrono::seconds(5));
```

---

#### get_send_timeout()

```cpp
std::chrono::milliseconds get_send_timeout();
```

Gets the send timeout.

**Returns:** Current send timeout

---

#### set_receive_timeout()

```cpp
void set_receive_timeout(const std::chrono::milliseconds& timeout);
```

Sets timeout for receive operations.

**Parameters:**
- `timeout` - Receive timeout (0 = blocking)

**Example:**
```cpp
stream.set_receive_timeout(std::chrono::seconds(10));
```

---

#### get_receive_timeout()

```cpp
std::chrono::milliseconds get_receive_timeout();
```

Gets the receive timeout.

**Returns:** Current receive timeout

---

### Status Queries

#### socket()

```cpp
tcp_client& socket();
const tcp_client& socket() const;
```

Accesses the underlying TCP socket.

**Returns:** Reference to tcp_client

**Example:**
```cpp
std::cout << "Connected to: " << stream.socket().remote_address().to_string() << "\n";
```

---

#### rdbuf()

```cpp
socket_stream_buf* rdbuf() const;
```

Accesses the underlying stream buffer.

**Returns:** Pointer to socket_stream_buf

**Example:**
```cpp
auto buf = stream.rdbuf();
std::cout << "Buffer size: " << buf->buffer_size() << "\n";
```

---

#### is_connected()

```cpp
bool is_connected() const;
```

Checks if the stream's socket is connected.

**Returns:** true if connected, false otherwise

**Example:**
```cpp
if (stream.is_connected()) {
    stream << "Still connected\n";
}
```

---

#### address()

```cpp
socket_address address() const;
```

Gets the local address.

**Returns:** Local socket_address

**Example:**
```cpp
std::cout << "Local: " << stream.address().to_string() << "\n";
```

---

#### peer_address()

```cpp
socket_address peer_address() const;
```

Gets the remote peer's address.

**Returns:** Remote socket_address

**Example:**
```cpp
std::cout << "Remote: " << stream.peer_address().to_string() << "\n";
```

---

#### buffer_size()

```cpp
std::size_t buffer_size() const;
```

Gets the stream buffer size.

**Returns:** Buffer size in bytes

---

#### set_buffer_size()

```cpp
void set_buffer_size(std::size_t size);
```

Changes the stream buffer size (buffers must be empty).

**Throws:** ``std::system_error`` if buffers not empty

---

#### close()

```cpp
void close();
```

Closes the underlying socket.

**Example:**
```cpp
stream.close();
```

---

## Stream-Based I/O

### Advantages over tcp_client

**With tcp_client:**
```cpp
tcp_client socket;
socket.connect(address);

std::string buffer;
socket.receive(buffer, 1024);

std::string response = "OK";
socket.send(response);
```

**With socket_stream:**
```cpp
socket_stream stream(address);

std::getline(stream, buffer);

stream << "OK\n";
```

### Input Formatting

```cpp
// Read structured data
int id;
std::string name;
double value;

stream >> id >> name >> value;
```

### Output Formatting

```cpp
// Format and send data
stream << std::setw(10) << std::left << "Column1"
       << std::setw(10) << std::left << "Column2\n";

stream << std::fixed << std::setprecision(2)
       << 3.14159 << "\n";  // Prints "3.14"
```

### String-Based Operations

```cpp
// Read entire line
std::string line;
std::getline(stream, line);

// Read until delimiter
std::getline(stream, line, ':');

// Read into container
std::vector<std::string> lines;
std::string line;
while (std::getline(stream, line)) {
    lines.push_back(line);
}
```

---

## Complete Examples

### Example 1: HTTP GET Request

```cpp
#include <fb/fb_net.h>
#include <iostream>

using namespace fb;

int main() {
    fb::initialize_library();

    socket_stream stream(socket_address("example.com", 80));

    // Send HTTP request
    stream << "GET /index.html HTTP/1.1\r\n"
           << "Host: example.com\r\n"
           << "User-Agent: FB_NET/1.0\r\n"
           << "Connection: close\r\n"
           << "\r\n";

    // Read response
    std::string line;
    int line_count = 0;
    while (std::getline(stream, line) && line_count < 20) {
        std::cout << line << "\n";
        line_count++;
    }

    stream.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 2: Interactive Telnet-Like Client

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <iomanip>

using namespace fb;

int main() {
    fb::initialize_library();

    std::string host = "localhost";
    int port = 8888;

    socket_stream stream(socket_address(host, port),
                        std::chrono::seconds(5));

    stream.set_no_delay(true);  // Interactive use
    stream.set_receive_timeout(std::chrono::seconds(10));

    std::string command;
    std::cout << "Connected to " << host << ":" << port << "\n";
    std::cout << "Type 'quit' to exit\n\n";

    while (std::cout << "> " && std::getline(std::cin, command)) {
        if (command == "quit") break;

        // Send command
        stream << command << "\n";
        stream.flush();

        // Read response
        std::string response;
        if (std::getline(stream, response)) {
            std::cout << "< " << response << "\n";
        } else {
            std::cout << "Server disconnected\n";
            break;
        }
    }

    stream.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 3: Configuration Protocol

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <iomanip>

using namespace fb;

struct Config {
    std::string name;
    int port;
    double timeout;
};

Config read_config_from_stream(socket_stream& stream) {
    Config cfg;

    // Read configuration in text format
    stream >> cfg.name >> cfg.port >> cfg.timeout;

    return cfg;
}

void send_config_to_stream(socket_stream& stream, const Config& cfg) {
    // Send configuration in text format
    stream << cfg.name << " " << cfg.port << " "
           << std::fixed << std::setprecision(2) << cfg.timeout << "\n";
    stream.flush();
}

int main() {
    fb::initialize_library();

    socket_stream stream(socket_address("config.server.local", 9999));

    // Read configuration
    Config cfg = read_config_from_stream(stream);
    std::cout << "Name: " << cfg.name << "\n"
              << "Port: " << cfg.port << "\n"
              << "Timeout: " << cfg.timeout << "s\n";

    stream.close();
    fb::cleanup_library();
    return 0;
}
```

---

### Example 4: Multi-Line Protocol

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <vector>

using namespace fb;

class SMTPClient {
    socket_stream stream;

public:
    SMTPClient(const socket_address& server)
        : stream(server, std::chrono::seconds(30))
    {
        stream.set_receive_timeout(std::chrono::seconds(10));
    }

    void send_mail(const std::string& from,
                  const std::string& to,
                  const std::string& subject,
                  const std::string& body)
    {
        // Receive greeting
        std::string response;
        std::getline(stream, response);
        std::cout << "Server: " << response << "\n";

        // Send EHLO
        stream << "EHLO client.local\r\n";
        stream.flush();

        // Receive response
        while (std::getline(stream, response)) {
            std::cout << "Server: " << response << "\n";
            if (response[3] == ' ') break;  // Last line
        }

        // Send MAIL FROM
        stream << "MAIL FROM:<" << from << ">\r\n";
        stream.flush();
        std::getline(stream, response);
        std::cout << "Server: " << response << "\n";

        // Send RCPT TO
        stream << "RCPT TO:<" << to << ">\r\n";
        stream.flush();
        std::getline(stream, response);
        std::cout << "Server: " << response << "\n";

        // Send DATA
        stream << "DATA\r\n";
        stream.flush();
        std::getline(stream, response);
        std::cout << "Server: " << response << "\n";

        // Send message
        stream << "Subject: " << subject << "\r\n"
               << "\r\n"
               << body << "\r\n.\r\n";
        stream.flush();
        std::getline(stream, response);
        std::cout << "Server: " << response << "\n";

        // QUIT
        stream << "QUIT\r\n";
        stream.flush();
    }
};

int main() {
    fb::initialize_library();

    SMTPClient client(socket_address("mail.example.com", 25));

    client.send_mail(
        "user@example.com",
        "recipient@example.com",
        "Test Email",
        "This is a test message"
    );

    fb::cleanup_library();
    return 0;
}
```

---

### Example 5: Bidirectional Stream with Error Handling

```cpp
#include <fb/fb_net.h>
#include <iostream>
#include <sstream>

using namespace fb;

int main() {
    fb::initialize_library();

    try {
        socket_stream stream(socket_address("localhost", 8888),
                            std::chrono::seconds(5));

        stream.set_no_delay(true);
        stream.set_receive_timeout(std::chrono::seconds(30));

        // Send request
        std::cout << "Sending request...\n";
        stream << "GET /api/status\n";
        stream.flush();

        // Read response
        std::cout << "Reading response...\n";
        std::string status_line;
        if (std::getline(stream, status_line)) {
            std::cout << "Status: " << status_line << "\n";

            // Parse status
            std::istringstream iss(status_line);
            std::string key, value;
            while (iss >> key >> value) {
                std::cout << "  " << key << " = " << value << "\n";
            }
        } else {
            std::cerr << "Failed to read response\n";
        }

        stream.close();

    } catch (const `std::system_error` (std::errc::timed_out)& ex) {
        std::cerr << "Timeout: " << ex.what() << "\n";
    } catch (const std::system_error& ex) {
        std::cerr << "Network error: " << ex.what() << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }

    fb::cleanup_library();
    return 0;
}
```

---

## Common Patterns

### Pattern 1: Request-Response Loop

```cpp
socket_stream stream(address);

std::string request;
while (std::cout << "Command> " && std::getline(std::cin, request)) {
    if (request.empty()) continue;
    if (request == "quit") break;

    // Send request
    stream << request << "\n";
    stream.flush();

    // Read response
    std::string response;
    if (std::getline(stream, response)) {
        std::cout << "Response: " << response << "\n";
    } else {
        std::cerr << "Connection closed\n";
        break;
    }
}
```

---

### Pattern 2: Line-Based Protocol

```cpp
socket_stream stream(address);

// Read header lines until empty line
std::string line;
while (std::getline(stream, line) && !line.empty()) {
    std::cout << "Header: " << line << "\n";
}

// Read body
std::string body_line;
while (std::getline(stream, body_line)) {
    std::cout << body_line << "\n";
}
```

---

### Pattern 3: Formatted Data Exchange

```cpp
socket_stream stream(address);

// Send structured data
struct Command {
    int id;
    std::string action;
    double value;
};

Command cmd{1, "set", 3.14};
stream << cmd.id << " " << cmd.action << " " << cmd.value << "\n";
stream.flush();

// Receive response
int response_id;
std::string response_text;
stream >> response_id >> response_text;
```

---

### Pattern 4: Buffering Control

```cpp
socket_stream stream(address);

// Manual buffer management
stream << "Part 1";           // Buffered
stream << " Part 2";          // Buffered
stream.flush();               // Send all buffered data

// Or use std::endl (flushes automatically)
stream << "Message" << std::endl;
```

---

## Performance Considerations

### Buffer Size Tuning

```cpp
// Large buffer for bulk transfers
socket_stream large_buf(address, 65536);  // 64KB

// Small buffer for interactive use
socket_stream small_buf(address, 1024);   // 1KB
```

**Guidelines:**
- **Interactive (HTTP, telnet):** 8KB-16KB (default 8KB)
- **Bulk transfer:** 32KB-64KB
- **Low-latency:** 1KB-4KB with TCP_NODELAY

---

### TCP_NODELAY for Interactivity

```cpp
socket_stream stream(address);
stream.set_no_delay(true);  // Disable Nagle for interactive use

stream << "Command\n";
stream.flush();  // Send immediately without buffering
```

---

### Timeout Configuration

```cpp
socket_stream stream(address, std::chrono::seconds(5));

stream.set_receive_timeout(std::chrono::seconds(30));
stream.set_send_timeout(std::chrono::seconds(10));
```

---

### Avoid std::endl (Use \n Instead)

```cpp
// ! Slow - flushes after every line
stream << "Line 1" << std::endl;
stream << "Line 2" << std::endl;

// ✅ Fast - buffers multiple lines
stream << "Line 1\n";
stream << "Line 2\n";
stream.flush();  // Flush once when done
```

---

## Error Handling

### Stream State Checking

```cpp
socket_stream stream(address);

if (!stream) {
    std::cerr << "Stream error\n";
}

stream << "Request\n";
if (!stream) {
    std::cerr << "Send error\n";
}

std::string response;
if (!std::getline(stream, response)) {
    std::cerr << "Receive error\n";
}
```

---

### Exception-Based Error Handling

```cpp
try {
    socket_stream stream(socket_address("server", 8888),
                        std::chrono::seconds(5));

    stream << "Request\n";
    stream.flush();

    std::string response;
    if (!std::getline(stream, response)) {
        throw std::runtime_error("Failed to read response");
    }

    stream.close();

} catch (const `std::system_error` (std::errc::timed_out)& ex) {
    std::cerr << "Timeout: " << ex.what() << "\n";
} catch (const std::system_error& ex) {
    std::cerr << "Network error: " << ex.what() << "\n";
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
}
```

---

## See Also

- [tcp_client](tcp_client.md) - Underlying low-level TCP socket
- [socket_address](socket_address.md) - Network address representation
- [Exception Handling](exceptions.md) - Network exception hierarchy
- C++ Standard Library `<iostream>` - Stream interface documentation

---

**[Back to Index](index.md)** | **[Previous: udp_client](udp_client.md)** | **[Next: socket_base](socket_base.md)**
