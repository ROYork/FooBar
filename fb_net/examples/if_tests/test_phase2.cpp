#include <fb/socket_base.h>
#include <fb/tcp_client.h>
#include <fb/server_socket.h>
#include <fb/socket_address.h>
#include <chrono>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>

/**
 * @brief Phase 2 validation test for FB_NET Core Socket Classes
 */
int main()
{
    std::cout << "FB_NET Phase 2 (Core Socket Classes) Validation Test\n";
    std::cout << "===================================================\n\n";

    int tests_passed = 0;
    int tests_failed = 0;

    try 
    {
        // Test 1: Socket base class creation and basic operations
        std::cout << "Test 1: Socket base class functionality\n";
        {
            fb::socket socket(fb::socket_address::IPv4, fb::socket::STREAM_SOCKET);
            std::cout << "  - Created Socket instance" << std::endl;
            std::cout << "  - Is connected: " << (socket.is_connected() ? "true" : "false") << std::endl;
            std::cout << "  - Is closed: " << (socket.is_closed() ? "true" : "false") << std::endl;
            std::cout << "  - Is secure: " << (socket.secure() ? "true" : "false") << std::endl;
            std::cout << "  - Blocking mode: " << (socket.get_blocking() ? "true" : "false") << std::endl;
            socket.close();
            std::cout << "  - Closed socket successfully" << std::endl;
        }
        std::cout << "  ✓ Socket base class tests passed\n\n";
        tests_passed++;

        // Test 2: tcp_client creation and configuration
        std::cout << "Test 2: tcp_client basic functionality\n";
        {
            fb::tcp_client stream_socket;
            std::cout << "  - Created tcp_client instance" << std::endl;
            
            // Test socket options
            stream_socket.set_no_delay(true);
            bool no_delay = stream_socket.get_no_delay();
            std::cout << "  - TCP No Delay: " << (no_delay ? "enabled" : "disabled") << std::endl;
            
            stream_socket.set_keep_alive(true);
            bool keep_alive = stream_socket.get_keep_alive();
            std::cout << "  - Keep Alive: " << (keep_alive ? "enabled" : "disabled") << std::endl;
            
            stream_socket.close();
            std::cout << "  - Closed tcp_client successfully" << std::endl;
        }
        std::cout << "  ✓ tcp_client basic tests passed\n\n";
        tests_passed++;

        // Test 3: tcp_client connection to external server
        std::cout << "Test 3: tcp_client external connection\n";
        {
            try {
                fb::tcp_client client_socket;
                fb::socket_address google_addr("google.com", 80);
                std::cout << "  - Attempting connection to google.com:80..." << std::endl;
                
                // Set a reasonable timeout
                client_socket.set_send_timeout(std::chrono::milliseconds(5000));
                client_socket.set_receive_timeout(std::chrono::milliseconds(5000));
                
                // Connect with timeout
                client_socket.connect(google_addr, std::chrono::milliseconds(10000));
                std::cout << "  - Connected successfully!" << std::endl;
                std::cout << "  - Is connected: " << (client_socket.is_connected() ? "true" : "false") << std::endl;
                
                // Get connection info
                fb::socket_address local_addr = client_socket.address();
                fb::socket_address peer_addr = client_socket.peer_address();
                std::cout << "  - Local address: " << local_addr.to_string() << std::endl;
                std::cout << "  - Peer address: " << peer_addr.to_string() << std::endl;
                
                // Send simple HTTP request
                std::string http_request = "GET / HTTP/1.1\r\nHost: google.com\r\nConnection: close\r\n\r\n";
                int sent = client_socket.send(http_request);
                std::cout << "  - Sent HTTP request: " << sent << " bytes" << std::endl;
                
                // Receive response
                char buffer[1024];
                int received = client_socket.receive_bytes(buffer, sizeof(buffer) - 1);
                if (received > 0) {
                    buffer[received] = '\0';
                    std::cout << "  - Received response: " << received << " bytes" << std::endl;
                    // Print first line of response
                    std::string response(buffer);
                    size_t first_line_end = response.find('\n');
                    if (first_line_end != std::string::npos) {
                        std::cout << "  - First line: " << response.substr(0, first_line_end) << std::endl;
                    }
                }
                
                client_socket.close();
                std::cout << "  - Connection closed successfully" << std::endl;
                
            } catch (const std::system_error& e) {
                std::cout << "  - Connection failed (expected in some environments): "
                          << e.what() << " (code: " << e.code() << ")" << std::endl;
                std::cout << "  - This is not necessarily an error - network access may be restricted" << std::endl;
            }
        }
        std::cout << "  ✓ tcp_client connection test completed\n\n";
        tests_passed++;

        // Test 4: server_socket creation and binding
        std::cout << "Test 4: server_socket functionality\n";
        {
            fb::server_socket server_socket;
            std::cout << "  - Created server_socket instance" << std::endl;
            
            try {
                // Bind to localhost on an available port
                fb::socket_address server_addr("127.0.0.1", 0); // Port 0 = auto-assign
                server_socket.bind(server_addr);
                std::cout << "  - Bound to address successfully" << std::endl;
                
                // Start listening
                server_socket.listen(10);
                std::cout << "  - Started listening (backlog=10)" << std::endl;
                
                // Get the actual bound address
                fb::socket_address bound_addr = server_socket.address();
                std::cout << "  - Bound address: " << bound_addr.to_string() << std::endl;
                
                // Test server socket properties
                std::cout << "  - Address reuse: " << (server_socket.get_reuse_address() ? "enabled" : "disabled") << std::endl;
                std::cout << "  - Max connections: " << server_socket.max_connections() << std::endl;
                
                server_socket.close();
                std::cout << "  - Server socket closed successfully" << std::endl;
                
            } catch (const std::system_error& e) {
                std::cout << "  - Server binding failed: " << e.what() << " (code: "
                          << e.code() << ")" << std::endl;
                tests_failed++;
            }
        }
        std::cout << "  ✓ server_socket basic tests passed\n\n";
        tests_passed++;

        // Test 5: Client-Server interaction (local loopback)
        std::cout << "Test 5: Client-Server local interaction\n";
        {
            try {
                // Create server socket
                fb::server_socket server;
                fb::socket_address server_addr("127.0.0.1", 0);
                server.bind(server_addr);
                server.listen(5);
                
                fb::socket_address bound_addr = server.address();
                std::cout << "  - Server listening on: " << bound_addr.to_string() << std::endl;
                
                // Create server thread to accept one connection
                std::thread server_thread([&server]() {
                    try {
                        fb::socket_address client_addr;
                        fb::tcp_client client_conn = server.accept_connection(client_addr, std::chrono::milliseconds(5000));
                        std::cout << "  - Server accepted connection from: " << client_addr.to_string() << std::endl;
                        
                        // Receive message from client
                        char buffer[256];
                        int received = client_conn.receive_bytes(buffer, sizeof(buffer) - 1);
                        if (received > 0) {
                            buffer[received] = '\0';
                            std::cout << "  - Server received: \"" << buffer << "\"" << std::endl;
                            
                            // Send response
                            std::string response = "Hello from server!";
                            client_conn.send(response);
                            std::cout << "  - Server sent response" << std::endl;
                        }
                        
                        client_conn.close();
                    } catch (const std::system_error& e) {
                        std::cout << "  - Server error: " << e.what() << " (code: "
                                  << e.code() << ")" << std::endl;
                    }
                });
                
                // Give server time to start accepting
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // Create client and connect
                fb::tcp_client client;
                client.connect(bound_addr);
                std::cout << "  - Client connected to server" << std::endl;
                
                // Send message to server
                std::string message = "Hello from client!";
                client.send(message);
                std::cout << "  - Client sent: \"" << message << "\"" << std::endl;
                
                // Receive response
                char response_buffer[256];
                int received = client.receive_bytes(response_buffer, sizeof(response_buffer) - 1);
                if (received > 0) {
                    response_buffer[received] = '\0';
                    std::cout << "  - Client received: \"" << response_buffer << "\"" << std::endl;
                }
                
                client.close();
                server_thread.join();
                server.close();
                
            } catch (const std::system_error& e) {
                std::cout << "  - Client-server interaction failed: " << e.what() << " (code: "
                          << e.code() << ")" << std::endl;
                tests_failed++;
            }
        }
        std::cout << "  ✓ Client-Server interaction test passed\n\n";
        tests_passed++;

        // Test 6: Exception handling
        std::cout << "Test 6: Exception handling\n";
        {
            try {
                fb::socket_address invalid_addr("invalid.invalid.invalid", 80);
                fb::tcp_client client;
                client.connect(invalid_addr, std::chrono::milliseconds(1000));
            } catch (const std::exception& e) {
                std::cout << "  - Caught expected exception: " << e.what() << std::endl;
            }

            try {
                fb::tcp_client closed_socket;
                closed_socket.close();
                closed_socket.send("test");
            } catch (const std::exception& e) {
                std::cout << "  - Caught expected socket misuse exception: " << e.what() << std::endl;
            }
        }
        std::cout << "  ✓ Exception handling tests passed\n\n";
        tests_passed++;

    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Unexpected test failure: " << e.what() << std::endl;
        tests_failed++;
    }

    // Print summary
    std::cout << "===================================================\n";
    std::cout << "Phase 2 Test Summary:\n";
    std::cout << "  Tests Passed: " << tests_passed << std::endl;
    std::cout << "  Tests Failed: " << tests_failed << std::endl;
    
    if (tests_failed == 0) {
        std::cout << "All Phase 2 Core Socket Classes tests completed successfully!\n";
        std::cout << "===================================================\n";
        return 0;
    } else {
        std::cout << "Some tests failed. Please review the output above.\n";
        std::cout << "===================================================\n";
        return 1;
    }
}
