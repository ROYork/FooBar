# fb_net Examples

Here are some practical examples of using the `fb_net` library. Each example is ready to compile, demonstrating real-world use cases.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Example 1: Simple TCP Client](#example-1-simple-tcp-client)
3. [Example 2: TCP Echo Server](#example-2-tcp-echo-server)
4. [Example 3: HTTP Client](#example-3-http-client)
5. [Example 4: Multi-threaded Chat Server](#example-4-multi-threaded-chat-server)
6. [Example 5: UDP Ping-Pong](#example-5-udp-ping-pong)
7. [Example 6: File Transfer Over TCP](#example-6-file-transfer-over-tcp)
8. [Example 7: Port Scanner](#example-7-port-scanner)
9. [Example 8: HTTP Server](#example-8-http-server)
10. [Best Practices](#best-practices)
11. [Integration Examples](#integration-examples)

---

## Getting Started

To use fb_net in your code, include the main header:

```cpp
#include <fb/fb_net.h>
using namespace fb;
```

All examples assume you have included the appropriate headers and are using the `fb` namespace.

**Important**: Always call `fb::initialize_library()` before using fb_net and `fb::cleanup_library()` when done (optional, but recommended).

---

## Example 1: Simple TCP Client

This example demonstrates basic TCP client functionality - connecting to a server and exchanging data.

### Scenario
Connect to a TCP server, send a message, and receive a response.

### Complete Code

```cpp
#include <iostream>
#include <fb/fb_net.h>

using namespace fb;

int main()
{
    // Initialize library
    initialize_library();

    try {
        // Create TCP client socket
        tcp_client socket(socket_address::IPv4);

        // Connect to server
        std::cout << "Connecting to server..." << std::endl;
        socket.connect(socket_address("127.0.0.1", 8080), std::chrono::seconds(5));
        std::cout << "Connected!" << std::endl;

        // Send message
        std::string message = "Hello, Server!";
        std::cout << "Sending: " << message << std::endl;
        socket.send(message);

        // Receive response
        std::string response;
        int bytes_received = socket.receive(response, 1024);
        std::cout << "Received " << bytes_received << " bytes: " << response << std::endl;

        // Socket automatically closed when it goes out of scope

    } catch (const `std::system_error` (std::errc::connection_refused)& e) {
        std::cerr << "Connection refused: " << e.what() << std::endl;
    } catch (const `std::system_error` (std::errc::timed_out)& e) {
        std::cerr << "Connection timeout: " << e.what() << std::endl;
    } catch (const std::system_error& e) {
        std::cerr << "Network error: " << e.what() << std::endl;
    }

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **tcp_client**: Create TCP socket for client connections
2. **connect()**: Establish connection with timeout
3. **send()**/**receive()**: Exchange data
4. **Exception handling**: Catch specific network errors
5. **RAII**: Automatic socket cleanup

---

## Example 2: TCP Echo Server

Create a simple TCP server that echoes received data back to clients.

### Scenario
Accept client connections and echo back any data received.

### Complete Code

```cpp
#include <iostream>
#include <fb/fb_net.h>

using namespace fb;

// Connection handler class
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

            // Read and echo data until client disconnects
            while (true) {
                int bytes = socket().receive(buffer, 1024);

                if (bytes <= 0) {
                    break;  // Client disconnected
                }

                std::cout << "Received " << bytes << " bytes from "
                         << client_address().to_string() << std::endl;

                // Echo back
                socket().send(buffer);
            }

        } catch (const ConnectionResetException& e) {
            std::cout << "Client reset connection" << std::endl;
        } catch (const std::system_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }

        std::cout << "Client disconnected: " << client_address().to_string() << std::endl;
    }
};

int main()
{
    initialize_library();

    try {
        // Create server socket
        server_socket srv_socket(socket_address::IPv4);
        srv_socket.bind(socket_address(8080));  // Bind to all interfaces
        srv_socket.listen(10);  // Backlog of 10

        std::cout << "Echo server listening on port 8080" << std::endl;

        // Connection factory
        auto factory = [](tcp_client socket, const socket_address& addr) {
            return std::make_unique<EchoConnection>(std::move(socket), addr);
        };

        // Create and start server
        tcp_server server(std::move(srv_socket), factory);
        server.set_max_threads(10);
        server.start();

        std::cout << "Server running. Press Enter to stop..." << std::endl;
        std::cin.get();

        // Stop server
        server.stop();
        std::cout << "Server stopped." << std::endl;

    } catch (const std::system_error& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **tcp_server_connection**: Base class for connection handlers
2. **server_socket**: Create listening socket
3. **tcp_server**: Multi-threaded server infrastructure
4. **Connection factory**: Lambda to create connection handlers
5. **Thread management**: Automatic thread pool handling

---

## Example 3: HTTP Client

Fetch web pages using HTTP protocol.

### Scenario
Make HTTP requests and parse responses.

### Complete Code

```cpp
#include <iostream>
#include <sstream>
#include <fb/fb_net.h>

using namespace fb;

class HTTPClient
{
public:
    struct Response {
        int status_code;
        std::string status_message;
        std::string headers;
        std::string body;
    };

    Response get(const std::string& host, int port, const std::string& path)
    {
        tcp_client socket(socket_address::IPv4);

        // Connect to server
        socket.connect(socket_address(host, port), std::chrono::seconds(10));

        // Build HTTP request
        std::ostringstream request;
        request << "GET " << path << " HTTP/1.1\r\n"
                << "Host: " << host << "\r\n"
                << "Connection: close\r\n"
                << "User-Agent: fb_net/1.0\r\n"
                << "\r\n";

        // Send request
        socket.send(request.str());

        // Receive response
        std::string full_response;
        std::string chunk;

        while (socket.receive(chunk, 4096) > 0) {
            full_response += chunk;
        }

        // Parse response
        return parse_response(full_response);
    }

private:
    Response parse_response(const std::string& response)
    {
        Response result;

        // Find header/body separator
        size_t header_end = response.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            throw std::runtime_error("Invalid HTTP response");
        }

        std::string headers = response.substr(0, header_end);
        result.body = response.substr(header_end + 4);

        // Parse status line
        size_t first_line_end = headers.find("\r\n");
        std::string status_line = headers.substr(0, first_line_end);
        result.headers = headers.substr(first_line_end + 2);

        // Extract status code
        std::istringstream iss(status_line);
        std::string http_version;
        iss >> http_version >> result.status_code;
        std::getline(iss, result.status_message);

        return result;
    }
};

int main()
{
    initialize_library();

    HTTPClient client;

    try {
        std::cout << "Fetching http://example.com/ ..." << std::endl;

        auto response = client.get("example.com", 80, "/");

        std::cout << "Status: " << response.status_code << " "
                  << response.status_message << std::endl;
        std::cout << "Body length: " << response.body.length() << " bytes" << std::endl;
        std::cout << "\nFirst 500 characters:\n"
                  << response.body.substr(0, 500) << std::endl;

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **HTTP protocol**: Construct HTTP requests manually
2. **Response parsing**: Extract status, headers, and body
3. **Connection close**: Read until server closes connection
4. **Error handling**: Catch and report network errors

---

## Example 4: Multi-threaded Chat Server

A complete chat server that broadcasts messages to all connected clients.

### Scenario
Multiple clients can connect, send messages, and receive broadcasts from other clients.

### Complete Code

```cpp
#include <iostream>
#include <vector>
#include <mutex>
#include <algorithm>
#include <fb/fb_net.h>

using namespace fb;

class ChatServer
{
public:
    class ChatConnection : public tcp_server_connection
    {
    public:
        ChatConnection(tcp_client socket, const socket_address& addr,
                      ChatServer* server, const std::string& name)
            : tcp_server_connection(std::move(socket), addr)
            , server_(server)
            , client_name_(name)
        {}

        void run() override
        {
            std::cout << client_name_ << " connected from "
                      << client_address().to_string() << std::endl;

            // Send welcome message
            send_message("Welcome to the chat, " + client_name_ + "!");
            server_->broadcast(client_name_ + " joined the chat", this);

            try {
                std::string buffer;

                while (socket().receive(buffer, 1024) > 0) {
                    // Broadcast message to all clients
                    server_->broadcast(client_name_ + ": " + buffer, this);
                    buffer.clear();
                }

            } catch (const std::system_error& e) {
                std::cerr << client_name_ << " error: " << e.what() << std::endl;
            }

            server_->broadcast(client_name_ + " left the chat", this);
            server_->remove_client(this);

            std::cout << client_name_ << " disconnected" << std::endl;
        }

        void send_message(const std::string& msg)
        {
            try {
                socket().send(msg + "\n");
            } catch (const std::system_error&) {
                // Client disconnected
            }
        }

    private:
        ChatServer* server_;
        std::string client_name_;
    };

    void add_client(ChatConnection* client)
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.push_back(client);
    }

    void remove_client(ChatConnection* client)
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.erase(std::remove(clients_.begin(), clients_.end(), client),
                      clients_.end());
    }

    void broadcast(const std::string& message, ChatConnection* sender)
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);

        std::cout << "Broadcasting: " << message << std::endl;

        for (auto* client : clients_) {
            if (client != sender) {  // Don't echo to sender
                client->send_message(message);
            }
        }
    }

private:
    std::vector<ChatConnection*> clients_;
    std::mutex clients_mutex_;
    int client_counter_ = 0;
};

int main()
{
    initialize_library();

    try {
        ChatServer chat_server;

        server_socket srv_socket(socket_address::IPv4);
        srv_socket.bind(socket_address(9090));
        srv_socket.listen();

        std::cout << "Chat server listening on port 9090" << std::endl;

        int client_id = 0;

        auto factory = [&chat_server, &client_id](tcp_client socket,
                                                   const socket_address& addr) {
            std::string name = "User" + std::to_string(++client_id);
            auto conn = std::make_unique<ChatServer::ChatConnection>(
                std::move(socket), addr, &chat_server, name);

            chat_server.add_client(conn.get());
            return conn;
        };

        tcp_server server(std::move(srv_socket), factory);
        server.set_max_threads(50);
        server.start();

        std::cout << "Server running. Press Enter to stop..." << std::endl;
        std::cin.get();

        server.stop();

    } catch (const std::system_error& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **Message broadcasting**: Send to all connected clients
2. **Thread-safe client list**: Mutex-protected client container
3. **Client lifecycle**: Track join/leave events
4. **Custom connection class**: Extend tcp_server_connection
5. **Server-side state**: Maintain server-wide data structures

---

## Example 5: UDP Ping-Pong

Demonstrate UDP datagrams with a simple ping-pong protocol.

### Scenario
Client sends "PING", server responds with "PONG". Measure round-trip time.

### Complete Code

```cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <fb/fb_net.h>

using namespace fb;

// UDP Server
void udp_server_example()
{
    try {
        udp_socket server_socket(socket_address::IPv4);
        server_socket.bind(socket_address(9000));

        std::cout << "UDP server listening on port 9000" << std::endl;

        char buffer[1024];
        socket_address client_addr;

        while (true) {
            int bytes = server_socket.receive_from(buffer, sizeof(buffer), client_addr);

            if (bytes > 0) {
                std::string message(buffer, bytes);
                std::cout << "Received from " << client_addr.to_string()
                         << ": " << message << std::endl;

                if (message == "PING") {
                    server_socket.send_to("PONG", 4, client_addr);
                }
            }
        }

    } catch (const std::system_error& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

// UDP Client
void udp_client_example()
{
    try {
        udp_socket client_socket(socket_address::IPv4);
        socket_address server_addr("127.0.0.1", 9000);

        // Measure round-trip time
        for (int i = 0; i < 10; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            // Send PING
            client_socket.send_to("PING", 4, server_addr);

            // Receive PONG
            char buffer[1024];
            socket_address from_addr;
            int bytes = client_socket.receive_from(buffer, sizeof(buffer), from_addr);

            auto end = std::chrono::high_resolution_clock::now();
            auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            if (bytes > 0) {
                std::string response(buffer, bytes);
                std::cout << "Round-trip " << i + 1 << ": " << response
                         << " (RTT: " << rtt.count() << " μs)" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

    } catch (const std::system_error& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
    }
}

int main()
{
    initialize_library();

    // Run server in separate thread (or separate process)
    std::thread server_thread(udp_server_example);
    server_thread.detach();

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Run client
    udp_client_example();

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **udp_socket**: Connectionless datagram protocol
2. **send_to()**/**receive_from()**: Datagram operations with addressing
3. **Round-trip timing**: Measure network latency
4. **Concurrent execution**: Server and client in separate threads

---

## Example 6: File Transfer Over TCP

Transfer files between client and server with progress tracking.

### Scenario
Client sends a file to server, showing upload progress. Server receives and saves the file.

### Complete Code

```cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fb/fb_net.h>

using namespace fb;
namespace fs = std::filesystem;

// Server: Receive file
class FileReceiveConnection : public tcp_server_connection
{
public:
    FileReceiveConnection(tcp_client socket, const socket_address& addr)
        : tcp_server_connection(std::move(socket), addr)
    {}

    void run() override
    {
        try {
            // Receive filename
            std::string filename;
            socket().receive(filename, 256);

            // Receive file size
            std::string size_str;
            socket().receive(size_str, 32);
            size_t file_size = std::stoull(size_str);

            std::cout << "Receiving file: " << filename << " (" << file_size << " bytes)"
                      << std::endl;

            // Open output file
            std::ofstream outfile("received_" + filename, std::ios::binary);
            if (!outfile) {
                throw std::runtime_error("Cannot create output file");
            }

            // Receive file data
            char buffer[4096];
            size_t received = 0;

            while (received < file_size) {
                int bytes = socket().receive_bytes(buffer, sizeof(buffer));
                if (bytes <= 0) break;

                outfile.write(buffer, bytes);
                received += bytes;

                // Progress
                int progress = (received * 100) / file_size;
                if (received % (file_size / 10) == 0) {  // Every 10%
                    std::cout << "Progress: " << progress << "%" << std::endl;
                }
            }

            outfile.close();

            if (received == file_size) {
                std::cout << "File received successfully!" << std::endl;
                socket().send("OK");
            } else {
                std::cout << "File transfer incomplete" << std::endl;
                socket().send("ERROR");
            }

        } catch (const std::system_error& e) {
            std::cerr << "Transfer error: " << e.what() << std::endl;
        }
    }
};

// Client: Send file
void send_file(const std::string& filename, const std::string& server_host, int server_port)
{
    try {
        // Check if file exists
        if (!fs::exists(filename)) {
            std::cerr << "File not found: " << filename << std::endl;
            return;
        }

        size_t file_size = fs::file_size(filename);

        // Connect to server
        tcp_client socket(socket_address::IPv4);
        socket.connect(socket_address(server_host, server_port), std::chrono::seconds(5));

        // Send filename
        fs::path path(filename);
        std::string base_name = path.filename().string();
        socket.send(base_name);

        // Send file size
        socket.send(std::to_string(file_size));

        std::cout << "Sending file: " << base_name << " (" << file_size << " bytes)"
                  << std::endl;

        // Open input file
        std::ifstream infile(filename, std::ios::binary);

        // Send file data
        char buffer[4096];
        size_t sent = 0;

        while (sent < file_size) {
            infile.read(buffer, sizeof(buffer));
            std::streamsize bytes_read = infile.gcount();

            socket.send_bytes(buffer, static_cast<int>(bytes_read));
            sent += bytes_read;

            // Progress
            int progress = (sent * 100) / file_size;
            std::cout << "\rProgress: " << progress << "%" << std::flush;
        }

        std::cout << std::endl;

        // Wait for acknowledgment
        std::string response;
        socket.receive(response, 32);

        if (response == "OK") {
            std::cout << "File sent successfully!" << std::endl;
        } else {
            std::cout << "Server reported error" << std::endl;
        }

    } catch (const std::system_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    initialize_library();

    if (argc < 2) {
        std::cout << "Usage:" << std::endl;
        std::cout << "  Server: " << argv[0] << " server <port>" << std::endl;
        std::cout << "  Client: " << argv[0] << " client <file> <host> <port>" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        int port = argc > 2 ? std::stoi(argv[2]) : 8080;

        server_socket srv_socket(socket_address::IPv4);
        srv_socket.bind(socket_address(port));
        srv_socket.listen();

        std::cout << "File server listening on port " << port << std::endl;

        auto factory = [](tcp_client socket, const socket_address& addr) {
            return std::make_unique<FileReceiveConnection>(std::move(socket), addr);
        };

        tcp_server server(std::move(srv_socket), factory);
        server.start();

        std::cout << "Press Enter to stop..." << std::endl;
        std::cin.get();
        server.stop();

    } else if (mode == "client" && argc >= 5) {
        std::string filename = argv[2];
        std::string host = argv[3];
        int port = std::stoi(argv[4]);

        send_file(filename, host, port);
    }

    cleanup_library();
    return 0;
}
```

### Key Techniques

1. **File I/O**: Read and write files
2. **Binary transfer**: Use `send_bytes()`/`receive_bytes()`
3. **Progress tracking**: Calculate and display progress
4. **Protocol design**: Send metadata before data
5. **Acknowledgment**: Confirm successful transfer

---

## Best Practices

### 1. Always Initialize the Library

```cpp
int main()
{
    fb::initialize_library();  // Required for Windows

    // Your code here

    fb::cleanup_library();  // Optional but recommended
    return 0;
}
```

---

### 2. Use Timeouts for Robustness

```cpp
// Good: Set timeouts to prevent blocking indefinitely
tcp_client socket;
socket.set_receive_timeout(std::chrono::seconds(30));
socket.connect(address, std::chrono::seconds(5));

// Bad: No timeout - may block forever
tcp_client socket;
socket.connect(address);
```

---

### 3. Handle Specific Exceptions

```cpp
// Good: Handle specific error cases
try {
    socket.connect(address);
} catch (const `std::system_error` (std::errc::connection_refused)& e) {
    // Try alternative server
} catch (const `std::system_error` (std::errc::timed_out)& e) {
    // Increase timeout and retry
} catch (const std::system_error& e) {
    // Log and abort
}

// Bad: Generic catch
try {
    socket.connect(address);
} catch (...) {
    // Can't determine what went wrong
}
```

---

### 4. Use RAII for Resource Management

```cpp
// Good: Automatic cleanup
void transfer_data()
{
    tcp_client socket;
    socket.connect(address);
    socket.send(data);
    // Socket automatically closed on exception or return
}

// Don't need explicit close() - destructor handles it
```

---

### 5. Implement Retry Logic for Transient Errors

```cpp
bool connect_with_retry(tcp_client& socket, const socket_address& addr,
                       int max_attempts = 3)
{
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            socket.connect(addr, std::chrono::seconds(5));
            return true;
        } catch (const `std::system_error` (std::errc::connection_refused)& e) {
            if (attempt < max_attempts) {
                std::this_thread::sleep_for(std::chrono::seconds(attempt));
            }
        }
    }
    return false;
}
```

---

## Integration Examples

### Integration with Logging Libraries

```cpp
#include <fb/fb_net.h>
#include <spdlog/spdlog.h>

void robust_connect(tcp_client& socket, const socket_address& addr)
{
    try {
        spdlog::info("Connecting to {}", addr.to_string());
        socket.connect(addr, std::chrono::seconds(5));
        spdlog::info("Connected successfully");

    } catch (const `std::system_error` (std::errc::timed_out)& e) {
        spdlog::error("Connection timeout: {}", e.what());
        throw;
    } catch (const std::system_error& e) {
        spdlog::error("Connection failed: {} (code: {})", e.what(), e.code());
        throw;
    }
}
```

---

### Integration with JSON Libraries

```cpp
#include <fb/fb_net.h>
#include <nlohmann/json.hpp>

void send_json(tcp_client& socket, const nlohmann::json& data)
{
    std::string json_str = data.dump();

    // Send length first
    uint32_t length = json_str.length();
    socket.send_bytes(&length, sizeof(length));

    // Send JSON data
    socket.send(json_str);
}

nlohmann::json receive_json(tcp_client& socket)
{
    // Receive length
    uint32_t length;
    socket.receive_bytes(&length, sizeof(length));

    // Receive JSON data
    std::string json_str;
    socket.receive(json_str, length);

    return nlohmann::json::parse(json_str);
}
```

---

### Integration with Threading Libraries

```cpp
#include <fb/fb_net.h>
#include <thread>
#include <atomic>

class NetworkMonitor
{
public:
    void start(const socket_address& target)
    {
        running_ = true;
        monitor_thread_ = std::thread(&NetworkMonitor::monitor_loop, this, target);
    }

    void stop()
    {
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

private:
    void monitor_loop(const socket_address& target)
    {
        while (running_) {
            try {
                tcp_client socket;
                socket.connect(target, std::chrono::seconds(2));
                socket.close();

                std::cout << "Server " << target.to_string() << " is UP" << std::endl;

            } catch (const std::system_error&) {
                std::cout << "Server " << target.to_string() << " is DOWN" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }

    std::atomic<bool> running_{false};
    std::thread monitor_thread_;
};
```

---

*fb_net Examples - Practical network programming with modern C++*
