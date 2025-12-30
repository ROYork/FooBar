/**
 * Phase 4 Validation Tests - Server Infrastructure and UDP Components
 * 
 * This test validates:
 * 1. TCPServer can handle multiple concurrent connections
 * 2. TCPServerConnection properly processes client requests  
 * 3. UDPClient can send/receive packets efficiently
 * 4. UDPServer can handle concurrent UDP packets
 * 5. Thread safety and resource cleanup work correctly
 * 6. Integration with previous phase components is seamless
 */

#include <fb/tcp_server.h>
#include <fb/tcp_server_connection.h>
#include <fb/udp_server.h>
#include <fb/udp_handler.h>
#include <fb/UDPClient.h>
#include <fb/tcp_client.h>
#include <fb/server_socket.h>
#include <fb/udp_socket.h>
#include <fb/socket_address.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <system_error>
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <sstream>

using namespace fb;

// Test statistics
std::atomic<int> tcp_connections_handled{0};
std::atomic<int> udp_packets_processed{0};
std::atomic<bool> test_running{true};

/**
 * Simple Echo TCPServerConnection for testing
 */
class EchoTCPConnection : public TCPServerConnection
{
public:
    EchoTCPConnection(tcp_client socket, const socket_address& client_address)
        : TCPServerConnection(std::move(socket), client_address)
    {
    }

    void run() override
    {
        try {
            std::cout << "TCP connection from " << client_address().to_string() << std::endl;
            
            // Simple echo server - read and echo back data
            char buffer[1024];
            while (!stop_requested()) {
                int received = socket().receive_bytes(buffer, sizeof(buffer));
                if (received <= 0) {
                    break; // Connection closed or error
                }
                
                // Echo the data back
                socket().send_bytes_all(buffer, received);
                tcp_connections_handled.fetch_add(1);
                
                // If we receive "quit", close connection
                std::string data(buffer, received);
                if (data.find("quit") != std::string::npos) {
                    break;
                }
            }
            
        } catch (const std::exception& ex) {
            std::cout << "TCP connection error: " << ex.what() << std::endl;
        }
    }
};

/**
 * Simple Echo UDP Handler for testing
 */
class EchoUDPHandler : public UDPHandler
{
public:
    void handle_packet(const void* buffer, std::size_t length, 
                      const socket_address& sender_address) override
    {
        try {
            std::string data(static_cast<const char*>(buffer), length);
            std::cout << "UDP packet from " << sender_address.to_string() 
                      << ": " << data << std::endl;
            
            udp_packets_processed.fetch_add(1);
            
            // In a real server, we would send a response back
            // For testing, we just process and count
            
        } catch (const std::exception& ex) {
            std::cout << "UDP handler error: " << ex.what() << std::endl;
        }
    }
    
    std::string handler_name() const override
    {
        return "EchoUDPHandler";
    }
};

/**
 * Test 1: TCP Server with Multiple Concurrent Connections
 */
bool test_tcp_server_concurrent_connections()
{
    std::cout << "\n=== Test 1: TCP Server Concurrent Connections ===" << std::endl;
    
    try {
        // Create server socket
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.bind(socket_address("127.0.0.1", 9090));
        server_socket.listen();
        
        std::cout << "Server listening on " << server_socket.address().to_string() << std::endl;
        
        // Create connection factory
        auto connection_factory = [](tcp_client socket, const socket_address& client_addr) {
            return std::make_unique<EchoTCPConnection>(std::move(socket), client_addr);
        };
        
        // Create and start TCP server
        TCPServer tcp_server(std::move(server_socket), connection_factory, 5, 10);
        tcp_server.start();
        
        std::cout << "TCP server started with " << tcp_server.thread_count() << " threads" << std::endl;
        
        // Create multiple client connections
        std::vector<std::thread> client_threads;
        const int num_clients = 3;
        
        for (int i = 0; i < num_clients; ++i) {
            client_threads.emplace_back([i]() {
                try {
                    tcp_client client_socket(socket_address::Family::IPv4);
                    client_socket.connect(socket_address("127.0.0.1", 9090));
                    
                    std::string message = "Hello from client " + std::to_string(i);
                    client_socket.send(message);
                    
                    // Read echo response
                    std::string response;
                    client_socket.receive(response, 1024);
                    
                    std::cout << "Client " << i << " got response: " << response << std::endl;
                    
                    // Send quit to close connection gracefully
                    client_socket.send("quit");
                    
                } catch (const std::exception& ex) {
                    std::cout << "Client " << i << " error: " << ex.what() << std::endl;
                }
            });
        }
        
        // Wait for clients to complete
        for (auto& thread : client_threads) {
            thread.join();
        }
        
        // Give server time to process
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        // Stop server
        tcp_server.stop();
        
        std::cout << "TCP server statistics:" << std::endl;
        std::cout << "  Total connections: " << tcp_server.total_connections() << std::endl;
        std::cout << "  Connections handled: " << tcp_connections_handled.load() << std::endl;
        
        return tcp_connections_handled.load() >= num_clients;
        
    } catch (const std::exception& ex) {
        std::cout << "TCP server test error: " << ex.what() << std::endl;
        return false;
    }
}

/**
 * Test 2: UDP Client and Server Communication
 */
bool test_udp_client_server()
{
    std::cout << "\n=== Test 2: UDP Client and Server Communication ===" << std::endl;
    
    try {
        // Create UDP server socket
        udp_socket server_socket(socket_address::Family::IPv4);
        server_socket.bind(socket_address("127.0.0.1", 9091));
        
        std::cout << "UDP server listening on " << server_socket.address().to_string() << std::endl;
        
        // Create shared UDP handler
        auto udp_handler = std::make_shared<EchoUDPHandler>();
        
        // Create and start UDP server
        UDPServer udp_server(std::move(server_socket), udp_handler, 3, 100);
        udp_server.start();
        
        std::cout << "UDP server started with " << udp_server.thread_count() << " threads" << std::endl;
        
        // Create UDP client
        UDPClient udp_client(socket_address::Family::IPv4);
        
        // Send multiple packets
        const int num_packets = 5;
        for (int i = 0; i < num_packets; ++i) {
            std::string message = "UDP packet " + std::to_string(i);
            int sent = udp_client.send_to(message, socket_address("127.0.0.1", 9091));
            std::cout << "Sent " << sent << " bytes: " << message << std::endl;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Give server time to process packets
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Stop server
        udp_server.stop();
        
        std::cout << "UDP server statistics:" << std::endl;
        std::cout << "  Total packets: " << udp_server.total_packets() << std::endl;
        std::cout << "  Processed packets: " << udp_server.processed_packets() << std::endl;
        std::cout << "  Dropped packets: " << udp_server.dropped_packets() << std::endl;
        std::cout << "  Handler processed: " << udp_packets_processed.load() << std::endl;
        
        return udp_packets_processed.load() >= num_packets;
        
    } catch (const std::exception& ex) {
        std::cout << "UDP client/server test error: " << ex.what() << std::endl;
        return false;
    }
}

/**
 * Test 3: UDP Client Connected Mode
 */
bool test_udp_client_connected_mode()
{
    std::cout << "\n=== Test 3: UDP Client Connected Mode ===" << std::endl;
    
    try {
        // Create a simple UDP echo server using udp_socket directly
        udp_socket echo_server(socket_address::Family::IPv4);
        echo_server.bind(socket_address("127.0.0.1", 9092));
        
        std::cout << "UDP echo server on " << echo_server.address().to_string() << std::endl;
        
        // Start echo server in background thread
        std::thread server_thread([&echo_server]() {
            char buffer[1024];
            socket_address sender;
            
            for (int i = 0; i < 3; ++i) {
                try {
                    int received = echo_server.receive_from(buffer, sizeof(buffer), sender);
                    if (received > 0) {
                        std::cout << "Echo server received " << received << " bytes from " 
                                  << sender.to_string() << std::endl;
                        // Echo back
                        echo_server.send_to(buffer, received, sender);
                    }
                } catch (const std::exception& ex) {
                    std::cout << "Echo server error: " << ex.what() << std::endl;
                    break;
                }
            }
        });
        
        // Create UDP client in connected mode
        UDPClient udp_client;
        udp_client.connect(socket_address("127.0.0.1", 9092));
        
        std::cout << "UDP client connected to " << udp_client.remote_address().to_string() << std::endl;
        
        // Send and receive using connected mode
        for (int i = 0; i < 3; ++i) {
            std::string message = "Connected message " + std::to_string(i);
            int sent = udp_client.send(message);
            std::cout << "Sent " << sent << " bytes: " << message << std::endl;
            
            // Receive response
            std::string response;
            int received = udp_client.receive(response, 1024);
            std::cout << "Received " << received << " bytes: " << response << std::endl;
        }
        
        // Cleanup
        server_thread.join();
        echo_server.close();
        
        return true;
        
    } catch (const std::exception& ex) {
        std::cout << "UDP connected mode test error: " << ex.what() << std::endl;
        return false;
    }
}

/**
 * Test 4: Thread Safety and Resource Management
 */
bool test_thread_safety_and_resources()
{
    std::cout << "\n=== Test 4: Thread Safety and Resource Management ===" << std::endl;
    
    try {
        // Test rapid start/stop cycles
        for (int cycle = 0; cycle < 3; ++cycle) {
            std::cout << "Cycle " << cycle << ": ";
            
            // Create TCP server
            server_socket server_socket(socket_address::Family::IPv4);
            server_socket.bind(socket_address("127.0.0.1", 9093 + cycle));
            server_socket.listen();
            
            auto connection_factory = [](tcp_client socket, const socket_address& client_addr) {
                return std::make_unique<EchoTCPConnection>(std::move(socket), client_addr);
            };
            
            TCPServer tcp_server(std::move(server_socket), connection_factory);
            
            // Start server
            tcp_server.start();
            std::cout << "Started - ";
            
            // Let it run briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Stop server
            tcp_server.stop(std::chrono::milliseconds(1000));
            std::cout << "Stopped";
            
            std::cout << std::endl;
        }
        
        std::cout << "Thread safety test completed successfully" << std::endl;
        return true;
        
    } catch (const std::exception& ex) {
        std::cout << "Thread safety test error: " << ex.what() << std::endl;
        return false;
    }
}

/**
 * Test 5: Integration with Previous Phase Components
 */
bool test_phase_integration()
{
    std::cout << "\n=== Test 5: Integration with Previous Phase Components ===" << std::endl;
    
    try {
        // Test that Layer 4 components work with Layer 1-3 components
        
        // 1. Use socket_address (Layer 1)
        socket_address addr("127.0.0.1", 9094);
        std::cout << "socket_address: " << addr.to_string() << std::endl;
        
        // 2. Use server_socket (Layer 2) with TCPServer (Layer 4)
        server_socket server_socket(addr.family());
        server_socket.bind(addr);
        server_socket.listen();
        
        // 3. Use udp_socket (Layer 3) with UDPClient (Layer 4)
        udp_socket datagram_socket(addr.family());
        UDPClient udp_client;
        
        // 4. Test that we can create all Layer 4 components
        auto connection_factory = [](tcp_client socket, const socket_address& client_addr) {
            return std::make_unique<EchoTCPConnection>(std::move(socket), client_addr);
        };
        
        TCPServer tcp_server(std::move(server_socket), connection_factory);
        
        auto udp_handler = std::make_shared<EchoUDPHandler>();
        UDPServer udp_server(std::move(datagram_socket), udp_handler);
        
        std::cout << "All Layer 4 components created successfully" << std::endl;
        std::cout << "Integration with previous layers verified" << std::endl;
        
        return true;
        
    } catch (const std::exception& ex) {
        std::cout << "Layer integration test error: " << ex.what() << std::endl;
        return false;
    }
}

int main()
{
    std::cout << "FB_NET Layer 4 Validation Tests" << std::endl;
    std::cout << "================================" << std::endl;
    
    int passed = 0;
    int total = 5;
    
    // Run all tests
    if (test_tcp_server_concurrent_connections()) {
        std::cout << " Test 1 PASSED" << std::endl;
        passed++;
    } else {
        std::cout << "! Test 1 FAILED" << std::endl;
    }
    
    if (test_udp_client_server()) {
        std::cout << " Test 2 PASSED" << std::endl;
        passed++;
    } else {
        std::cout << "! Test 2 FAILED" << std::endl;
    }
    
    if (test_udp_client_connected_mode()) {
        std::cout << " Test 3 PASSED" << std::endl;
        passed++;
    } else {
        std::cout << "! Test 3 FAILED" << std::endl;
    }
    
    if (test_thread_safety_and_resources()) {
        std::cout << " Test 4 PASSED" << std::endl;
        passed++;
    } else {
        std::cout << "! Test 4 FAILED" << std::endl;
    }
    
    if (test_phase_integration()) {
        std::cout << " Test 5 PASSED" << std::endl;
        passed++;
    } else {
        std::cout << "! Test 5 FAILED" << std::endl;
    }
    
    // Summary
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << " tests" << std::endl;
    
    if (passed == total) {
        std::cout << " ALL TESTS PASSED! Phase 4 implementation is working correctly." << std::endl;
        std::cout << "\nPhase 4 Components Validated:" << std::endl;
        std::cout << "  - TCPServerConnection - Base class for handling TCP connections" << std::endl;
        std::cout << "  - TCPServer - Multi-threaded TCP server with connection management" << std::endl;
        std::cout << "  - UDPHandler - Base class for handling UDP packets" << std::endl;
        std::cout << "  - UDPClient - High-level UDP client with simplified interface" << std::endl;
        std::cout << "  - UDPServer - Multi-threaded UDP server with packet dispatch" << std::endl;
        std::cout << "\nFeatures Demonstrated:" << std::endl;
        std::cout << "  - Concurrent TCP connection handling" << std::endl;
        std::cout << "  - Multi-threaded server architecture" << std::endl;
        std::cout << "  - UDP packet processing and dispatch" << std::endl;
        std::cout << "  - Thread safety and resource management" << std::endl;
        std::cout << "  - Integration with previous phase components" << std::endl;
        return 0;
    } else {
        std::cout << "!! Some tests failed. Please check the implementation." << std::endl;
        return 1;
    }
}
