/**
 * @file udp_ping_server.cpp
 * @brief UDP Ping Server Example using FB_NET
 * 
 * This example demonstrates a UDP ping server that responds to ping requests
 * from clients. It showcases udp_server and udp_handler usage for handling
 * multiple concurrent UDP clients.
 * 
 * Features demonstrated:
 * - UDPServer with custom UDPHandler implementation
 * - Multi-threaded UDP packet processing
 * - Ping/pong protocol over UDP
 * - Statistics tracking and reporting
 * - Timestamp calculations for round-trip time
 * 
 * Usage:
 *   ./udp_ping_server [port]
 *   Default port: 8888
 *   
 *   Test with: ./udp_ping_client localhost 8888
 */

#include <fb/fb_net.h>
#include <iostream>
#include <string>
#include <chrono>
#include <atomic>
#include <thread>
#include <sstream>
#include <iomanip>

using namespace fb;

/**
 * @brief Ping Handler - processes ping requests and sends pong responses
 */
class PingHandler : public udp_handler
{
public:
    PingHandler() : m_total_pings(0), m_start_time(std::chrono::steady_clock::now()) {}

    void handle_packet(const void* buffer, std::size_t length, 
                      const socket_address& sender_address) override
    {
        try {
            std::string request(static_cast<const char*>(buffer), length);
            
            // Parse ping request: "PING <sequence> <timestamp>"
            std::istringstream iss(request);
            std::string command;
            int sequence;
            long long client_timestamp;
            
            if (!(iss >> command >> sequence >> client_timestamp) || command != "PING") {
                std::cout << "[INVALID] Malformed ping from " << sender_address.to_string() 
                          << ": " << request << std::endl;
                return;
            }
            
            // Get current timestamp
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto server_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
            
            // Create pong response: "PONG <sequence> <client_timestamp> <server_timestamp>"
            std::ostringstream response;
            response << "PONG " << sequence << " " << client_timestamp << " " << server_timestamp;
            
            // Send response back to client (we need a way to send back)
            // Note: In a real implementation, we'd need access to the server socket
            // For this example, we'll just log the response
            std::cout << "[PING] " << sender_address.to_string() 
                      << " seq=" << sequence << " (would respond: " << response.str() << ")" << std::endl;
            
            m_total_pings.fetch_add(1);
            
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Ping handler error: " << ex.what() << std::endl;
        }
    }
    
    std::string handler_name() const override
    {
        return "PingHandler";
    }
    
    std::uint64_t total_pings() const
    {
        return m_total_pings.load();
    }
    
    std::chrono::steady_clock::duration uptime() const
    {
        return std::chrono::steady_clock::now() - m_start_time;
    }

private:
    std::atomic<std::uint64_t> m_total_pings;
    std::chrono::steady_clock::time_point m_start_time;
};

/**
 * @brief Enhanced Ping Server with direct response capability
 */
class PingServer
{
public:
    PingServer(int port) : m_port(port), m_running(false), m_total_pings(0) {}

    bool start()
    {
        try {
            m_socket = udp_socket(socket_address::Family::IPv4);
            m_socket.set_reuse_address(true);
            m_socket.bind(socket_address("0.0.0.0", static_cast<std::uint16_t>(m_port)));
            
            m_running = true;
            m_start_time = std::chrono::steady_clock::now();
            
            std::cout << "UDP Ping Server listening on " << m_socket.address().to_string() << std::endl;
            std::cout << "Ready to handle ping requests..." << std::endl;
            
            return true;
        } catch (const std::exception& ex) {
            std::cerr << "Failed to start server: " << ex.what() << std::endl;
            return false;
        }
    }

    void run()
    {
        char buffer[1024];
        
        while (m_running) {
            try {
                socket_address sender;
                int received = m_socket.receive_from(buffer, sizeof(buffer) - 1, sender);
                
                if (received > 0) {
                    buffer[received] = '\0';
                    handle_ping_request(std::string(buffer), sender);
                }
                
            } catch (const std::exception& ex) {
                if (m_running) {
                    std::cout << "[ERROR] Server error: " << ex.what() << std::endl;
                }
            }
        }
    }

    void stop()
    {
        m_running = false;
        try {
            m_socket.close();
        } catch (...) {
            // Ignore close errors
        }
    }

    std::uint64_t total_pings() const { return m_total_pings.load(); }
    
    std::chrono::steady_clock::duration uptime() const 
    { 
        return std::chrono::steady_clock::now() - m_start_time; 
    }

private:
    int m_port;
    udp_socket m_socket;
    std::atomic<bool> m_running;
    std::atomic<std::uint64_t> m_total_pings;
    std::chrono::steady_clock::time_point m_start_time;

    void handle_ping_request(const std::string& request, const socket_address& sender)
    {
        try {
            // Parse ping request: "PING <sequence> <timestamp>"
            std::istringstream iss(request);
            std::string command;
            int sequence;
            long long client_timestamp;
            
            if (!(iss >> command >> sequence >> client_timestamp) || command != "PING") {
                std::cout << "[INVALID] Malformed ping from " << sender.to_string() 
                          << ": " << request << std::endl;
                return;
            }
            
            // Get current timestamp
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto server_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(now).count();
            
            // Create pong response: "PONG <sequence> <client_timestamp> <server_timestamp>"
            std::ostringstream response;
            response << "PONG " << sequence << " " << client_timestamp << " " << server_timestamp;
            
            // Send response back to client
            std::string response_str = response.str();
            m_socket.send_to(response_str, sender);
            
            std::cout << "[PING] " << sender.to_string() 
                      << " seq=" << std::setfill('0') << std::setw(4) << sequence 
                      << " responded" << std::endl;
            
            m_total_pings.fetch_add(1);
            
        } catch (const std::exception& ex) {
            std::cout << "[ERROR] Failed to handle ping from " << sender.to_string() 
                      << ": " << ex.what() << std::endl;
        }
    }
};

int main(int argc, char* argv[])
{
    std::cout << "FB_NET UDP Ping Server Example" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Initialize FB_NET library
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        int port = 8888;
        if (argc > 1) {
            port = std::atoi(argv[1]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Invalid port number. Using default port 8888." << std::endl;
                port = 8888;
            }
        }
        
        // Create and start ping server
        PingServer server(port);
        if (!server.start()) {
            return 1;
        }
        
        std::cout << "Test with: ./udp_ping_client localhost " << port << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;
        
        // Start statistics thread
        std::atomic<bool> stats_running{true};
        std::thread stats_thread([&server, &stats_running]() {
            while (stats_running.load()) {
                std::this_thread::sleep_for(std::chrono::seconds(10));
                if (stats_running.load()) {
                    auto uptime_sec =
                        std::chrono::duration_cast<std::chrono::seconds>(server.uptime()).count();
                    std::cout << "[STATS] Total pings: " << server.total_pings()
                              << ", Uptime: " << uptime_sec << "s";
                    if (uptime_sec > 0) {
                        const auto rate = static_cast<double>(server.total_pings()) /
                                          static_cast<double>(uptime_sec);
                        const auto previous_flags = std::cout.flags();
                        const auto previous_precision = std::cout.precision();
                        std::cout << ", Rate: " << std::fixed << std::setprecision(2) << rate
                                  << " pings/sec";
                        std::cout.flags(previous_flags);
                        std::cout.precision(previous_precision);
                    }
                    std::cout << std::endl;
                }
            }
        });
        
        // Run server (blocks until stopped)
        server.run();
        
        // Cleanup
        stats_running.store(false);
        stats_thread.join();
        server.stop();
        
        std::cout << "\nServer stopped. Final statistics:" << std::endl;
        auto final_uptime =
            std::chrono::duration_cast<std::chrono::seconds>(server.uptime()).count();
        std::cout << "Total pings handled: " << server.total_pings() << std::endl;
        std::cout << "Total uptime: " << final_uptime << " seconds" << std::endl;
        if (final_uptime > 0) {
            const auto rate = static_cast<double>(server.total_pings()) /
                              static_cast<double>(final_uptime);
            const auto previous_flags = std::cout.flags();
            const auto previous_precision = std::cout.precision();
            std::cout << "Average rate: " << std::fixed << std::setprecision(2) << rate
                      << " pings/second" << std::endl;
            std::cout.flags(previous_flags);
            std::cout.precision(previous_precision);
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    
    // Cleanup FB_NET library
    fb::cleanup_library();
    return 0;
}
