# FB_NET Examples

This directory contains comprehensive examples demonstrating real-world usage of FB_NET components. Each example showcases different aspects of the library and can be used as a starting point for your own projects.

## 📁 Example Applications

### 1. [HTTP Server](http_server/) 
**Demonstrates**: TCPServer, TCPServerConnection, HTTP protocol handling

A full-featured HTTP server that handles multiple concurrent connections and serves static content with proper HTTP responses.

**Features**:
- Multi-threaded connection handling
- HTTP/1.1 protocol support
- Virtual hosting capabilities
- Static file serving
- JSON API endpoints

**Usage**:
```bash
./http_server [port]
# Default: port 8080
# Visit: http://localhost:8080
```

### 2. [Chat Server & Client](chat_server/)
**Demonstrates**: Multi-client TCP communication, message broadcasting, thread safety

A real-time chat system supporting multiple clients with features like user management, message broadcasting, and command processing.

**Features**:
- Multi-client support
- Real-time message broadcasting
- User commands (/list, /help, /quit)
- Connection management
- Thread-safe client handling

**Usage**:
```bash
# Start server
./chat_server [port]

# Connect clients  
./chat_client [host] [port]
```

### 3. [UDP Ping Client/Server](udp_ping/)
**Demonstrates**: UDP communication, UDPServer, UDPHandler, performance measurement

Network diagnostics tools that measure round-trip times and packet loss using UDP.

**Features**:
- RTT measurement with microsecond precision
- Packet loss detection
- Statistics collection
- Connected and connectionless UDP modes
- Performance benchmarking

**Usage**:
```bash
# Start ping server
./udp_ping_server [port]

# Run ping client
./udp_ping_client [host] [port] [count] [interval_ms]
```

### 4. [File Transfer System](file_transfer/)
**Demonstrates**: Large data transfers, socket_stream, binary data handling, progress tracking

File upload/download system with support for large files and progress monitoring.

**Features**:
- Binary file transfers
- Upload and download operations
- Progress tracking
- File integrity verification
- Directory management
- Transfer resumption support

**Usage**:
```bash
# Start file server
./file_server [port] [directory]

# Connect file client
./file_client [host] [port]
# Commands: LIST, DOWNLOAD <file>, UPLOAD <file> <size>
```

### 5. [Port Scanner](port_scanner/)
**Demonstrates**: poll_set usage, concurrent connections, non-blocking I/O

High-performance port scanner that can efficiently scan thousands of ports using polling and concurrent connections.

**Features**:
- Concurrent port scanning
- poll_set for efficient I/O multiplexing
- Service detection
- Timeout handling
- Progress reporting
- Statistics collection

**Usage**:
```bash
./port_scanner <host> <start_port> <end_port> [max_concurrent]
# Example: ./port_scanner localhost 20 80 50
```

## 🏗️ Building Examples

### Prerequisites
- FB_NET library installed
- CMake 3.16+
- C++17/20 compatible compiler

### Build Process

```bash
# From fb_net root directory
mkdir build && cd build

# Configure with examples enabled
cmake .. -DFB_NET_BUILD_EXAMPLES=ON

# Build all examples
make -j$(nproc)

# Examples will be in build/examples/
ls examples/
```

### Individual Example Build

```bash
# Build specific example
make http_server
make chat_server chat_client
make udp_ping_server udp_ping_client
make file_server file_client
make port_scanner
```

## 🧪 Testing Examples

Each example includes built-in error handling and can be tested independently:

### HTTP Server Test
```bash
# Terminal 1: Start server
./examples/http_server/http_server 8080

# Terminal 2: Test with curl
curl http://localhost:8080/
curl http://localhost:8080/api/status
```

### Chat System Test
```bash
# Terminal 1: Start server
./examples/chat_server/chat_server 9090

# Terminal 2-4: Connect multiple clients
./examples/chat_server/chat_client localhost 9090
```

### UDP Ping Test
```bash
# Terminal 1: Start server
./examples/udp_ping/udp_ping_server 8888

# Terminal 2: Run client
./examples/udp_ping/udp_ping_client localhost 8888 10 1000
```

### File Transfer Test
```bash
# Terminal 1: Start server
mkdir files
echo "Test content" > files/test.txt
./examples/file_transfer/file_server 8080 files

# Terminal 2: Connect client
./examples/file_transfer/file_client localhost 8080
# Commands: LIST, DOWNLOAD test.txt
```

### Port Scanner Test
```bash
# Scan local ports (requires running services)
./examples/port_scanner/port_scanner localhost 20 1024 100
```

## 📖 Code Structure

All examples follow consistent patterns:

### Common Structure
```cpp
#include <fb/fb_net.h>
#include <iostream>
// Other includes...

using namespace fb;

int main(int argc, char* argv[]) {
    // Initialize FB_NET
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        // Set up networking components
        // Run main application logic
        
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }
    
    // Cleanup FB_NET
    fb::cleanup_library();
    return 0;
}
```

### Server Pattern
```cpp
// 1. Create server socket
ServerSocket server_socket(socket_address::Family::IPv4);
server_socket.bind(socket_address("0.0.0.0", port));
server_socket.listen();

// 2. Define connection factory
auto factory = [](tcp_client socket, const socket_address& addr) {
    return std::make_unique<CustomConnection>(std::move(socket), addr);
};

// 3. Create and start server
TCPServer server(std::move(server_socket), factory, max_threads, max_queued);
server.start();
```

### Client Pattern
```cpp
// 1. Create client socket
tcp_client socket(socket_address::Family::IPv4);

// 2. Connect to server
socket.connect(socket_address(host, port));

// 3. Exchange data
socket.send("request");
std::string response;
socket.receive(response, buffer_size);
```

## 🛠️ Customization

Each example can be easily modified and extended:

### Adding Features
- Modify connection handlers for custom protocols
- Add authentication and security
- Implement custom message formats
- Add logging and monitoring
- Integrate with databases or other services

### Performance Tuning
- Adjust thread pool sizes
- Modify buffer sizes
- Tune timeout values
- Optimize data structures
- Add connection pooling

### Error Handling
- Add custom exception handlers
- Implement retry logic
- Add graceful shutdown
- Improve error reporting
- Add health checks

## 📚 Learning Path

**Beginner**: Start with HTTP Server to understand basic TCP server concepts

**Intermediate**: Try Chat Server for multi-client handling and real-time communication

**Advanced**: Explore Port Scanner for poll_set usage and concurrent I/O patterns

**Expert**: Study File Transfer for large data handling and binary protocols

## 💡 Best Practices Demonstrated

- **Resource Management**: RAII patterns, proper cleanup
- **Exception Safety**: Comprehensive error handling
- **Thread Safety**: Proper synchronization, atomic operations
- **Performance**: Efficient I/O, connection pooling
- **Scalability**: Multi-threading, non-blocking operations
- **Maintainability**: Clean code structure, good documentation

## 🔗 Related Documentation

- [FB_NET README](../README.md) - Main library documentation
- [API Reference](../include/fb/fb_net.h) - Complete API documentation
- [Integration Tests](../test_integration.cpp) - Test examples
- [Performance Benchmarks](../benchmark_performance.cpp) - Performance validation

---

These examples demonstrate FB_NET's capabilities and provide templates for building robust, high-performance network applications.