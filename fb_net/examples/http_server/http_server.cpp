/**
 * @file http_server.cpp
 * @brief Simple HTTP Server Example using FB_NET
 * 
 * This example demonstrates how to create a basic HTTP server using FB_NET's
 * tcp_server and tcp_server_connection classes. The server handles HTTP GET requests
 * and serves simple responses with basic HTML content.
 * 
 * Features demonstrated:
 * - tcp_server usage with connection factory
 * - tcp_server_connection implementation for HTTP protocol
 * - Socket stream operations for HTTP request/response handling
 * - Multi-threaded connection handling
 * - Basic HTTP protocol parsing and response generation
 * 
 * Usage:
 *   ./http_server [port]
 *   Default port: 8080
 *   
 *   Test with: curl http://localhost:8080/ or web browser
 */

#include <fb/fb_net.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <memory>
#include <thread>
#include <system_error>

using namespace fb;

/**
 * @brief HTTP Server Connection Handler
 * 
 * Handles individual HTTP client connections by parsing HTTP requests
 * and generating appropriate responses.
 */
class HTTPConnection : public tcp_server_connection
{
public:
    HTTPConnection(tcp_client socket, const socket_address& client_address)
        : tcp_server_connection(std::move(socket), client_address)
    {
        std::cout << "[" << get_timestamp() << "] New connection from " 
                  << client_address.to_string() << std::endl;
    }

    void run() override
    {
        try {
            // Set socket timeout
            socket().set_receive_timeout(std::chrono::seconds(30));
            socket().set_send_timeout(std::chrono::seconds(30));
            
            while (!stop_requested()) {
                // Read HTTP request
                std::string request = read_http_request();
                if (request.empty()) {
                    break; // Connection closed or timeout
                }
                
                // Parse request
                HTTPRequest req = parse_http_request(request);
                std::cout << "[" << get_timestamp() << "] " 
                          << client_address().to_string() << " - " 
                          << req.method << " " << req.path << std::endl;
                
                // Generate response
                std::string response = generate_http_response(req);
                
                // Send response
                const auto response_size = response.size();
                if (response_size > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
                    throw std::length_error("HTTP response exceeds maximum socket send size");
                }
                socket().send_bytes_all(response.c_str(), static_cast<int>(response_size));
                
                // Close connection for HTTP/1.0 or if Connection: close
                if (req.version == "HTTP/1.0" || req.connection == "close") {
                    break;
                }
            }
        }
        catch (const std::system_error&) {
            std::cout << "[" << get_timestamp() << "] Connection timeout: " 
                      << client_address().to_string() << std::endl;
        }
        catch (const std::exception& ex) {
            std::cout << "[" << get_timestamp() << "] Connection error: " 
                      << ex.what() << std::endl;
        }
        
        std::cout << "[" << get_timestamp() << "] Connection closed: " 
                  << client_address().to_string() << std::endl;
    }

private:
    struct HTTPRequest {
        std::string method;
        std::string path;
        std::string version;
        std::string connection;
        std::string host;
        std::string user_agent;
    };

    /**
     * @brief Read complete HTTP request from socket
     * @return HTTP request string or empty if connection closed
     */
    std::string read_http_request()
    {
        std::string request;
        std::string line;
        bool headers_complete = false;
        
        while (!headers_complete && !stop_requested()) {
            char ch;
            int received = socket().receive_bytes(&ch, 1);
            if (received <= 0) {
                return ""; // Connection closed
            }
            
            request += ch;
            
            // Check for end of headers (\r\n\r\n)
            if (request.size() >= 4) {
                std::string last4 = request.substr(request.size() - 4);
                if (last4 == "\r\n\r\n") {
                    headers_complete = true;
                }
            }
        }
        
        return request;
    }

    /**
     * @brief Parse HTTP request headers
     * @param request Raw HTTP request string
     * @return Parsed HTTPRequest structure
     */
    HTTPRequest parse_http_request(const std::string& request)
    {
        HTTPRequest req;
        req.connection = "close"; // Default
        
        std::istringstream iss(request);
        std::string line;
        
        // Parse request line
        if (std::getline(iss, line)) {
            std::istringstream line_iss(line);
            line_iss >> req.method >> req.path >> req.version;
        }
        
        // Parse headers
        while (std::getline(iss, line) && line != "\r") {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string header = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                
                // Trim whitespace
                while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
                    value.erase(0, 1);
                }
                while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
                    value.pop_back();
                }
                
                if (header == "Connection") {
                    req.connection = value;
                } else if (header == "Host") {
                    req.host = value;
                } else if (header == "User-Agent") {
                    req.user_agent = value;
                }
            }
        }
        
        return req;
    }

    /**
     * @brief Generate HTTP response for request
     * @param req Parsed HTTP request
     * @return Complete HTTP response string
     */
    std::string generate_http_response(const HTTPRequest& req)
    {
        std::ostringstream response;
        std::string content;
        std::string status = "200 OK";
        std::string content_type = "text/html";
        
        if (req.path == "/" || req.path == "/index.html") {
            content = generate_home_page();
        } else if (req.path == "/about") {
            content = generate_about_page();
        } else if (req.path == "/api/status") {
            content = generate_status_json();
            content_type = "application/json";
        } else {
            status = "404 Not Found";
            content = generate_404_page();
        }
        
        // HTTP response headers
        response << "HTTP/1.1 " << status << "\r\n";
        response << "Server: FB_NET HTTP Server 1.0\r\n";
        response << "Content-Type: " << content_type << "; charset=UTF-8\r\n";
        response << "Content-Length: " << content.length() << "\r\n";
        response << "Connection: " << req.connection << "\r\n";
        response << "Date: " << get_http_date() << "\r\n";
        response << "\r\n";
        response << content;
        
        return response.str();
    }

    std::string generate_home_page()
    {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>FB_NET HTTP Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { color: #2c3e50; border-bottom: 2px solid #3498db; }
        .info { background: #ecf0f1; padding: 20px; border-radius: 5px; margin: 20px 0; }
        .code { background: #2c3e50; color: #ecf0f1; padding: 10px; border-radius: 3px; }
        a { color: #3498db; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1 class="header">FB_NET HTTP Server</h1>
    <div class="info">
        <h2>Welcome to FB_NET!</h2>
        <p>This is a simple HTTP server built with the FB_NET networking library.</p>
        <p><strong>Server Status:</strong> Running</p>
        <p><strong>Library Version:</strong> )" + std::string(fb::Version::string) + R"(</p>
        <p><strong>Connections:</strong> Multi-threaded with connection pooling</p>
    </div>
    
    <h3>Available Endpoints:</h3>
    <ul>
        <li><a href="/">/</a> - This home page</li>
        <li><a href="/about">/about</a> - About FB_NET</li>
        <li><a href="/api/status">/api/status</a> - Server status (JSON)</li>
    </ul>
    
    <div class="info">
        <h3>Example cURL Commands:</h3>
        <div class="code">
            curl http://localhost:8080/<br>
            curl http://localhost:8080/about<br>
            curl http://localhost:8080/api/status
        </div>
    </div>
    
    <p><em>Powered by FB_NET - A cross-platform C++ networking library</em></p>
</body>
</html>)";
    }

    std::string generate_about_page()
    {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>About FB_NET</title>
    <style>body { font-family: Arial, sans-serif; margin: 40px; }</style>
</head>
<body>
    <h1>About FB_NET</h1>
    <p>FB_NET is a comprehensive, cross-platform C++ networking library that provides:</p>
    <ul>
        <li>Legacy Net API compatibility</li>
        <li>Modern C++20 features with C++17 fallback</li>
        <li>Thread-safe socket operations</li>
        <li>High-performance TCP and UDP networking</li>
        <li>Multi-threaded server architectures</li>
        <li>Cross-platform support (Windows, macOS, Linux)</li>
    </ul>
    <h2>Key Components:</h2>
    <ul>
        <li><strong>Foundation Layer:</strong> socket_address, socket, standard exceptions</li>
        <li><strong>Core Sockets:</strong> socket, tcp_client, server_socket</li>
        <li><strong>Advanced I/O:</strong> socket_stream, poll_set, udp_socket</li>
        <li><strong>Server Infrastructure:</strong> tcp_server, udp_server, Connection handlers</li>
    </ul>
    <p><a href="/">← Back to Home</a></p>
</body>
</html>)";
    }

    std::string generate_status_json()
    {
        auto uptime = std::chrono::steady_clock::now().time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
        
        return R"({
    "status": "running",
    "server": "FB_NET HTTP Server",
    "version": ")" + std::string(fb::Version::string) + R"(",
    "uptime_seconds": )" + std::to_string(seconds) + R"(,
    "thread_model": "multi-threaded",
    "platform": ")" + get_platform_name() + R"(",
    "connections": "active"
})";
    }

    std::string generate_404_page()
    {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>404 Not Found</title>
    <style>body { font-family: Arial, sans-serif; margin: 40px; text-align: center; }</style>
</head>
<body>
    <h1>404 Not Found</h1>
    <p>The requested resource was not found on this server.</p>
    <p><a href="/">← Back to Home</a></p>
</body>
</html>)";
    }

    std::string get_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_value = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_value);
        
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        return std::string(buffer);
    }

    std::string get_http_date()
    {
        auto now = std::chrono::system_clock::now();
        auto time_value = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::gmtime(&time_value);
        
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
        return std::string(buffer);
    }

    std::string get_platform_name()
    {
#ifdef FB_NET_PLATFORM_WINDOWS
        return "Windows";
#elif defined(FB_NET_PLATFORM_MACOS)
        return "macOS";
#elif defined(FB_NET_PLATFORM_LINUX)
        return "Linux";
#else
        return "Unknown";
#endif
    }
};

int main(int argc, char* argv[])
{
    std::cout << "FB_NET HTTP Server Example" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Initialize FB_NET library
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        int port = 8080;
        if (argc > 1) {
            port = std::atoi(argv[1]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Invalid port number. Using default port 8080." << std::endl;
                port = 8080;
            }
        }
        
        // Create server socket
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.set_reuse_address(true);
        server_socket.bind(socket_address("0.0.0.0", static_cast<std::uint16_t>(port)));
        server_socket.listen(128);
        
        std::cout << "Server listening on " << server_socket.address().to_string() << std::endl;
        std::cout << "Visit http://localhost:" << port << "/ in your web browser" << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;
        
        // Connection factory
        auto connection_factory = [](tcp_client socket, const socket_address& client_addr) {
            return std::make_unique<HTTPConnection>(std::move(socket), client_addr);
        };
        
        // Create and start TCP server with connection pooling
        tcp_server server(std::move(server_socket), connection_factory, 20, 100);
        server.start();
        
        std::cout << "HTTP Server started with " << server.thread_count() << " threads" << std::endl;
        std::cout << "Server uptime will be tracked from now..." << std::endl;
        
        // Keep server running until interrupted
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // Print server statistics every 10 seconds
            std::cout << "[Stats] Active connections: " << server.active_connections()
                      << ", Total handled: " << server.total_connections() << std::endl;
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    
    // Cleanup FB_NET library
    fb::cleanup_library();
    return 0;
}
