/**
 * @file udp_ping_client.cpp
 * @brief UDP Ping Client Example using FB_NET
 * 
 * This example demonstrates a UDP ping client that sends ping requests to
 * the UDP ping server and measures round-trip times. It showcases udp_client
 * usage and demonstrates both connected and connectionless UDP operations.
 * 
 * Features demonstrated:
 * - udp_client for sending UDP packets
 * - Round-trip time measurement
 * - Packet loss detection
 * - Statistics collection and reporting
 * - Timeout handling
 * 
 * Usage:
 *   ./udp_ping_client [host] [port] [count] [interval_ms]
 *   Defaults: localhost 8888 10 1000
 */

#include <fb/fb_net.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>
#include <system_error>

using namespace fb;

/**
 * @brief Ping Statistics Tracker
 */
class PingStats
{
public:
    void add_response(double rtt_ms)
    {
        m_rtts.push_back(rtt_ms);
        m_total_rtt += rtt_ms;
        m_min_rtt = (std::min)(m_min_rtt, rtt_ms);
        m_max_rtt = (std::max)(m_max_rtt, rtt_ms);
        m_received++;
    }
    
    void add_timeout()
    {
        m_timeouts++;
    }
    
    void increment_sent()
    {
        m_sent++;
    }
    
    void print_summary() const
    {
        std::cout << "\n=== Ping Statistics ===" << std::endl;
        std::cout << "Packets sent: " << m_sent << std::endl;
        std::cout << "Packets received: " << m_received << std::endl;
        std::cout << "Packets lost: " << (m_sent - m_received) << std::endl;
        
        if (m_sent > 0) {
            const double loss_pct =
                (static_cast<double>(m_sent - m_received) / static_cast<double>(m_sent)) * 100.0;
            std::cout << "Packet loss: " << std::fixed << std::setprecision(1) 
                      << loss_pct << "%" << std::endl;
        }
        
        if (m_received > 0) {
            double avg_rtt = m_total_rtt / m_received;
            std::cout << "Round-trip times:" << std::endl;
            std::cout << "  Minimum: " << std::fixed << std::setprecision(3) 
                      << m_min_rtt << " ms" << std::endl;
            std::cout << "  Maximum: " << std::fixed << std::setprecision(3) 
                      << m_max_rtt << " ms" << std::endl;
            std::cout << "  Average: " << std::fixed << std::setprecision(3) 
                      << avg_rtt << " ms" << std::endl;
            
            // Calculate standard deviation
            double variance = 0.0;
            for (double rtt : m_rtts) {
                variance += (rtt - avg_rtt) * (rtt - avg_rtt);
            }
            variance /= m_received;
            double stddev = std::sqrt(variance);
            
            std::cout << "  Std Dev: " << std::fixed << std::setprecision(3) 
                      << stddev << " ms" << std::endl;
        }
    }

private:
    int m_sent = 0;
    int m_received = 0;
    int m_timeouts = 0;
    double m_total_rtt = 0.0;
    double m_min_rtt = (std::numeric_limits<double>::max)();
    double m_max_rtt = 0.0;
    std::vector<double> m_rtts;
};

/**
 * @brief UDP Ping Client
 */
class PingClient
{
public:
    PingClient(const std::string& host, int port, int count, int interval_ms)
        : m_host(host), m_port(port), m_count(count), m_interval_ms(interval_ms)
    {
    }

    bool run()
    {
        try {
            // Create UDP client
            udp_client client(socket_address::Family::IPv4);
            client.set_timeout(std::chrono::milliseconds(5000)); // 5 second timeout
            
            socket_address target(m_host, static_cast<std::uint16_t>(m_port));
            
            std::cout << "PING " << target.to_string() << " (" << m_count << " packets)" << std::endl;
            std::cout << "Timeout: 5000ms, Interval: " << m_interval_ms << "ms" << std::endl;
            std::cout << std::endl;
            
            PingStats stats;
            
            for (int seq = 1; seq <= m_count; ++seq) {
                // Send ping
                auto start_time = std::chrono::steady_clock::now();
                auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                    start_time.time_since_epoch()).count();
                
                std::ostringstream ping_msg;
                ping_msg << "PING " << seq << " " << timestamp;
                std::string ping_str = ping_msg.str();
                
                try {
                    int sent = client.send_to(ping_str, target);
                    if (sent != static_cast<int>(ping_str.length())) {
                        std::cout << "seq=" << seq << " partial send (" << sent 
                                  << "/" << ping_str.length() << " bytes)" << std::endl;
                        continue;
                    }
                    
                    stats.increment_sent();
                    
                    // Wait for response
                    std::string response;
                    socket_address sender;
                    int received = client.receive_from(response, 1024, sender);
                    
                    auto end_time = std::chrono::steady_clock::now();
                    auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
                        end_time - start_time);
                    double rtt_ms = static_cast<double>(rtt.count()) / 1000.0;
                    
                    if (received > 0) {
                        // Parse response: "PONG <sequence> <client_timestamp> <server_timestamp>"
                        std::istringstream iss(response);
                        std::string command;
                        int resp_seq;
                        long long client_ts, server_ts;
                        
                        if (iss >> command >> resp_seq >> client_ts >> server_ts && 
                            command == "PONG" && resp_seq == seq && client_ts == timestamp) {
                            
                            stats.add_response(rtt_ms);
                            
                            std::cout << "seq=" << std::setfill('0') << std::setw(4) << seq 
                                      << " from " << sender.to_string()
                                      << " time=" << std::fixed << std::setprecision(3) 
                                      << rtt_ms << "ms" << std::endl;
                        } else {
                            std::cout << "seq=" << seq << " invalid response: " << response << std::endl;
                        }
                    }
                    
                } catch (const std::system_error&) {
                    stats.add_timeout();
                    std::cout << "seq=" << seq << " timeout (>5000ms)" << std::endl;
                } catch (const std::exception& ex) {
                    std::cout << "seq=" << seq << " error: " << ex.what() << std::endl;
                }
                
                // Wait before next ping (except for last one)
                if (seq < m_count) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
                }
            }
            
            // Print final statistics
            stats.print_summary();
            return true;
            
        } catch (const std::exception& ex) {
            std::cerr << "Ping client error: " << ex.what() << std::endl;
            return false;
        }
    }

private:
    std::string m_host;
    int m_port;
    int m_count;
    int m_interval_ms;
};

/**
 * @brief Test connected mode UDP client
 */
void test_connected_mode(const std::string& host, int port)
{
    std::cout << "\n=== Testing Connected Mode ===" << std::endl;
    
    try {
        udp_client client;
        client.connect(socket_address(host, static_cast<std::uint16_t>(port)));
        client.set_timeout(std::chrono::milliseconds(2000));
        
        std::cout << "Connected to " << client.remote_address().to_string() << std::endl;
        
        // Send a few pings using connected mode
        for (int i = 1; i <= 3; ++i) {
            auto start_time = std::chrono::steady_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                start_time.time_since_epoch()).count();
            
            std::ostringstream ping_msg;
            ping_msg << "PING " << (1000 + i) << " " << timestamp;
            std::string ping_str = ping_msg.str();
            
            try {
                client.send(ping_str);
                
                std::string response;
                int received = client.receive(response, 1024);
                
                if (received > 0) {
                    auto end_time = std::chrono::steady_clock::now();
                    auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(
                        end_time - start_time);
                    double rtt_ms = static_cast<double>(rtt.count()) / 1000.0;
                    
                    std::cout << "connected seq=" << (1000 + i) 
                              << " time=" << std::fixed << std::setprecision(3) 
                              << rtt_ms << "ms" << std::endl;
                }
                
            } catch (const std::system_error&) {
                std::cout << "connected seq=" << (1000 + i) << " timeout" << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
    } catch (const std::exception& ex) {
        std::cout << "Connected mode test error: " << ex.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    std::cout << "FB_NET UDP Ping Client Example" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Initialize FB_NET library
    fb::initialize_library();
    
    try {
        // Parse command line arguments
        std::string host = "localhost";
        int port = 8888;
        int count = 10;
        int interval_ms = 1000;
        
        if (argc > 1) host = argv[1];
        if (argc > 2) port = std::atoi(argv[2]);
        if (argc > 3) count = std::atoi(argv[3]);
        if (argc > 4) interval_ms = std::atoi(argv[4]);
        
        // Validate arguments
        if (port <= 0 || port > 65535) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
        if (count <= 0 || count > 1000) {
            std::cerr << "Invalid count (1-1000)" << std::endl;
            return 1;
        }
        if (interval_ms < 10 || interval_ms > 10000) {
            std::cerr << "Invalid interval (10-10000ms)" << std::endl;
            return 1;
        }
        
        // Run ping test
        PingClient client(host, port, count, interval_ms);
        bool success = client.run();
        
        // Test connected mode as bonus
        test_connected_mode(host, port);
        
        if (!success) {
            return 1;
        }
        
    } catch (const std::exception& ex) {
        std::cerr << "Client error: " << ex.what() << std::endl;
        return 1;
    }
    
    // Cleanup FB_NET library
    fb::cleanup_library();
    return 0;
}
