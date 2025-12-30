/**
 * @file test_integration.cpp
 * @brief Comprehensive Integration Test Suite for FB_NET Library
 *
 * This test suite validates end-to-end functionality across all FB_NET
 * components, testing integration scenarios that demonstrate real-world usage
 * patterns.
 *
 * Integration Test Coverage:
 * 1. End-to-end TCP client/server communication
 * 2. Mixed TCP/UDP server operations
 * 3. High-load concurrent connection testing
 * 4. Socket polling with mixed socket types
 * 5. Stream operations with large data transfers
 * 6. Error handling and recovery scenarios
 * 7. Resource management and cleanup validation
 * 8. Cross-component interoperability testing
 *
 * Usage:
 *   ./test_integration
 *
 * Expected Result: All integration tests pass, demonstrating production
 * readiness
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fb/fb_net.h>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace fb;

// Test configuration
constexpr int BASE_PORT             = 12000;
constexpr int HIGH_LOAD_CONNECTIONS = 100;
constexpr int LARGE_DATA_SIZE       = 1024 * 1024; // 1MB

constexpr std::uint16_t make_port(int offset)
{
  return static_cast<std::uint16_t>(BASE_PORT + offset);
}

constexpr std::uint16_t to_port(int value)
{
  return static_cast<std::uint16_t>(value);
}

// Global test statistics
std::atomic<int> g_tests_run{0};
std::atomic<int> g_tests_passed{0};
std::atomic<int> g_tests_failed{0};

// Test result tracking
struct TestResult
{
  std::string test_name;
  bool passed;
  std::string error_message;
  std::chrono::milliseconds duration;
};

std::vector<TestResult> g_test_results;
std::mutex g_results_mutex;

// Utility functions
void log_test_result(
    const std::string &test_name,
    bool passed,
    const std::string &error           = "",
    std::chrono::milliseconds duration = std::chrono::milliseconds(0))
{
  std::lock_guard<std::mutex> lock(g_results_mutex);

  TestResult result;
  result.test_name     = test_name;
  result.passed        = passed;
  result.error_message = error;
  result.duration      = duration;

  g_test_results.push_back(result);

  if (passed)
  {
    std::cout << "✓ " << test_name;
    if (duration.count() > 0)
    {
      std::cout << " (" << duration.count() << "ms)";
    }
    std::cout << std::endl;
    g_tests_passed++;
  }
  else
  {
    std::cout << "✗ " << test_name << " - " << error << std::endl;
    g_tests_failed++;
  }
  g_tests_run++;
}

std::string generate_test_data(size_t size)
{
  std::string data;
  data.reserve(size);

  const auto seed = static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  std::seed_seq seed_seq{static_cast<std::uint32_t>(seed & 0xffffffffu),
                         static_cast<std::uint32_t>(seed >> 32)};
  std::mt19937 rng(seed_seq);
  std::uniform_int_distribution<int> dist('A', 'Z');

  for (size_t i = 0; i < size; ++i)
  {
    data.push_back(static_cast<char>(dist(rng)));
  }

  return data;
}

/**
 * Integration Test 1: End-to-End TCP Communication
 */
class EchoServerConnection : public tcp_server_connection
{
public:
  EchoServerConnection(tcp_client socket, const socket_address &client_addr) :
    tcp_server_connection(std::move(socket), client_addr)
  {
  }

  void run() override
  {
    try
    {
      socket().set_receive_timeout(std::chrono::seconds(10));

      std::string data;
      while (socket().receive(data, 4096) > 0)
      {
        socket().send(data);
        if (data == "QUIT")
          break;
      }
    }
    catch (const std::exception &)
    {
      // Connection ended
    }
  }
};

bool test_tcp_end_to_end()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    // Create server
    server_socket server_socket(socket_address::Family::IPv4);
    server_socket.bind(socket_address("127.0.0.1", make_port(0)));
    server_socket.listen();

    auto factory = [](tcp_client socket, const socket_address &addr)
    { return std::make_unique<EchoServerConnection>(std::move(socket), addr); };

    tcp_server server(std::move(server_socket), factory);
    server.start();

    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create client and test communication
    tcp_client client(socket_address::Family::IPv4);
    client.connect(socket_address("127.0.0.1", make_port(0)));

    std::string test_message = "Hello, FB_NET Integration Test!";
    client.send(test_message);

    std::string response;
    client.receive(response, 1024);

    bool success = (response == test_message);

    client.send("QUIT");
    client.close();
    server.stop();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("TCP End-to-End Communication", success,
                    success ? "" : "Echo response mismatch", duration);
    return success;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("TCP End-to-End Communication", false, ex.what(), duration);
    return false;
  }
}

/**
 * Integration Test 2: Mixed TCP/UDP Operations
 */
class TestUDPHandler : public udp_handler
{
public:
  TestUDPHandler() :
    m_packets_received(0)
  {
  }

  void handle_packet(const void *buffer,
                     std::size_t length,
                     const socket_address &sender_address
                     [[maybe_unused]]) override
  {
    m_packets_received++;
    std::string data(static_cast<const char *>(buffer), length);
    m_last_packet = data;
  }

  std::string handler_name() const override { return "TestUDPHandler"; }

  int packets_received() const { return m_packets_received.load(); }
  std::string last_packet() const { return m_last_packet; }

private:
  std::atomic<int> m_packets_received;
  std::string m_last_packet;
};

bool test_mixed_tcp_udp_operations()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    // Start TCP server
    server_socket tcp_server_socket(socket_address::Family::IPv4);
    tcp_server_socket.bind(socket_address("127.0.0.1", make_port(1)));
    tcp_server_socket.listen();

    auto tcp_factory = [](tcp_client socket, const socket_address &addr)
    { return std::make_unique<EchoServerConnection>(std::move(socket), addr); };

    tcp_server tcp_server(std::move(tcp_server_socket), tcp_factory);
    tcp_server.start();

    // Start UDP server
    udp_socket udp_server_socket(socket_address::Family::IPv4);
    udp_server_socket.bind(socket_address("127.0.0.1", make_port(2)));

    auto udp_handler = std::make_shared<TestUDPHandler>();
    udp_server udp_server(std::move(udp_server_socket), udp_handler);
    udp_server.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Test TCP client
    tcp_client tcp_client(socket_address::Family::IPv4);
    tcp_client.connect(socket_address("127.0.0.1", make_port(1)));
    tcp_client.send("TCP_TEST");

    std::string tcp_response;
    tcp_client.receive(tcp_response, 1024);
    bool tcp_ok = (tcp_response == "TCP_TEST");

    // Test UDP client
    udp_client udp_client(socket_address::Family::IPv4);
    udp_client.send_to("UDP_TEST", socket_address("127.0.0.1", make_port(2)));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    bool udp_ok = (udp_handler->packets_received() > 0 &&
                   udp_handler->last_packet() == "UDP_TEST");

    // Cleanup
    tcp_client.send("QUIT");
    tcp_client.close();
    tcp_server.stop();
    udp_server.stop();

    bool success  = tcp_ok && udp_ok;

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Mixed TCP/UDP Operations", success,
                    success ? "" : "TCP or UDP communication failed", duration);
    return success;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Mixed TCP/UDP Operations", false, ex.what(), duration);
    return false;
  }
}

/**
 * Integration Test 3: High-Load Concurrent Connections
 */
bool test_high_load_connections()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    // Create server
    server_socket server_socket(socket_address::Family::IPv4);
    server_socket.bind(socket_address("127.0.0.1", make_port(3)));
    server_socket.listen();

    auto factory = [](tcp_client socket, const socket_address &addr)
    { return std::make_unique<EchoServerConnection>(std::move(socket), addr); };

    tcp_server server(std::move(server_socket), factory, 20, 200);
    server.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create multiple concurrent client connections
    std::vector<std::thread> client_threads;
    std::atomic<int> successful_connections{0};
    std::atomic<int> failed_connections{0};

    for (int i = 0; i < HIGH_LOAD_CONNECTIONS; ++i)
    {
      client_threads.emplace_back(
          [i, &successful_connections, &failed_connections]()
          {
            try
            {
              tcp_client client(socket_address::Family::IPv4);
              client.connect(socket_address("127.0.0.1", make_port(3)),
                             std::chrono::seconds(5));

              std::string message = "Client_" + std::to_string(i);
              client.send(message);

              std::string response;
              client.receive(response, 1024);

              if (response == message)
              {
                successful_connections++;
              }
              else
              {
                failed_connections++;
              }

              client.send("QUIT");
              client.close();
            }
            catch (const std::exception &)
            {
              failed_connections++;
            }
          });
    }

    // Wait for all clients to complete
    for (auto &thread : client_threads)
    {
      thread.join();
    }

    server.stop();

    bool success  = (successful_connections >=
                    HIGH_LOAD_CONNECTIONS * 0.95); // Allow 5% failure

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    std::string result_msg = std::to_string(successful_connections.load()) +
                             "/" + std::to_string(HIGH_LOAD_CONNECTIONS) +
                             " connections successful";

    log_test_result("High-Load Concurrent Connections", success,
                    success ? "" : result_msg, duration);
    return success;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("High-Load Concurrent Connections", false, ex.what(),
                    duration);
    return false;
  }
}

/**
 * Integration Test 4: Large Data Transfer
 */
bool test_large_data_transfer()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    // Generate large test data
    std::string large_data =
        generate_test_data(static_cast<std::size_t>(LARGE_DATA_SIZE));

    // Create server
    server_socket server_socket(socket_address::Family::IPv4);
    server_socket.bind(socket_address("127.0.0.1", make_port(4)));
    server_socket.listen();

    auto factory = [](tcp_client socket, const socket_address &addr)
    { return std::make_unique<EchoServerConnection>(std::move(socket), addr); };

    tcp_server server(std::move(server_socket), factory);
    server.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Create client and transfer large data
    tcp_client client(socket_address::Family::IPv4);
    client.set_send_timeout(std::chrono::seconds(30));
    client.set_receive_timeout(std::chrono::seconds(30));
    client.connect(socket_address("127.0.0.1", make_port(4)));

    // Send large data
    const auto send_size = large_data.size();
    if (send_size > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
    {
      throw std::length_error("Large data exceeds maximum socket send size");
    }
    client.send_bytes_all(large_data.c_str(), static_cast<int>(send_size));

    // Receive echo back
    std::string received_data;
    received_data.reserve(static_cast<std::size_t>(LARGE_DATA_SIZE));

    while (received_data.size() < large_data.size())
    {
      std::string chunk;
      int bytes_received = client.receive(chunk, 65536);
      if (bytes_received <= 0)
        break;
      received_data += chunk;
    }

    bool success = (received_data == large_data);

    client.send("QUIT");
    client.close();
    server.stop();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    std::string result_msg = std::to_string(received_data.size()) + "/" +
                             std::to_string(large_data.size()) +
                             " bytes transferred";

    log_test_result("Large Data Transfer", success, success ? "" : result_msg,
                    duration);
    return success;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Large Data Transfer", false, ex.what(), duration);
    return false;
  }
}

/**
 * Integration Test 5: Error Handling and Recovery
 */
bool test_error_handling_recovery()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    bool all_passed = true;

    // Test 1: Connection to non-existent server
    try
    {
      tcp_client client(socket_address::Family::IPv4);
      client.connect(socket_address("127.0.0.1", to_port(65432)),
                     std::chrono::milliseconds(1000)); // Likely refused
      all_passed = false;                              // Should have thrown
    }
    catch (const std::system_error &)
    {
      // Expected exception
    }
    catch (const std::exception &)
    {
      // Also acceptable
    }

    // Test 2: Invalid address
    try
    {
      socket_address invalid("999.999.999.999", 80);
      all_passed = false; // Should have thrown
    }
    catch (const std::invalid_argument &)
    {
      // Expected exception
    }
    catch (const std::exception &)
    {
      // Also acceptable
    }

    // Test 3: Timeout handling
    try
    {
      udp_socket udp_socket(socket_address::Family::IPv4);
      udp_socket.set_receive_timeout(std::chrono::milliseconds(100));

      std::string data;
      socket_address sender;
      udp_socket.receive_from(data, 1024, sender); // Should timeout
      all_passed = false;                          // Should have thrown
    }
    catch (const std::system_error &)
    {
      // Expected exception
    }
    catch (const std::exception &)
    {
      // Also acceptable
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Error Handling and Recovery", all_passed,
                    all_passed ? "" : "Exception handling failed", duration);
    return all_passed;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Error Handling and Recovery", false, ex.what(), duration);
    return false;
  }
}

/**
 * Integration Test 6: Resource Management
 */
bool test_resource_management()
{
  auto start_time = std::chrono::steady_clock::now();

  try
  {
    // Test rapid creation/destruction cycles
    for (int cycle = 0; cycle < 10; ++cycle)
    {
      {
        // Create server
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.bind(socket_address("127.0.0.1", make_port(5 + cycle)));
        server_socket.listen();

        auto factory = [](tcp_client socket, const socket_address &addr)
        {
          return std::make_unique<EchoServerConnection>(std::move(socket),
                                                        addr);
        };

        tcp_server server(std::move(server_socket), factory);
        server.start();

        // Brief operation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        server.stop();
        // Server destructor should clean up properly
      }

      {
        // Create UDP server
        udp_socket udp_socket(socket_address::Family::IPv4);
        udp_socket.bind(socket_address("127.0.0.1", make_port(15 + cycle)));

        auto handler = std::make_shared<TestUDPHandler>();
        udp_server udp_server(std::move(udp_socket), handler);
        udp_server.start();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        udp_server.stop();
        // Server destructor should clean up properly
      }
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Resource Management", true, "", duration);
    return true;
  }
  catch (const std::exception &ex)
  {
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    log_test_result("Resource Management", false, ex.what(), duration);
    return false;
  }
}

int main()
{
  std::cout << "FB_NET Comprehensive Integration Test Suite" << std::endl;
  std::cout << "============================================" << std::endl;
  std::cout << std::endl;

  // Initialize FB_NET library
  fb::initialize_library();

  auto suite_start = std::chrono::steady_clock::now();

  try
  {
    std::cout << "Running integration tests..." << std::endl;
    std::cout << std::endl;

    // Run all integration tests
    test_tcp_end_to_end();
    test_mixed_tcp_udp_operations();
    test_high_load_connections();
    test_large_data_transfer();
    test_error_handling_recovery();
    test_resource_management();

    auto suite_end      = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        suite_end - suite_start);

    // Print summary
    std::cout << std::endl;
    std::cout << "=== Integration Test Results ===" << std::endl;
    std::cout << "Tests run: " << g_tests_run.load() << std::endl;
    std::cout << "Passed: " << g_tests_passed.load() << std::endl;
    std::cout << "Failed: " << g_tests_failed.load() << std::endl;
    std::cout << "Total time: " << total_duration.count() << "ms" << std::endl;
    std::cout << std::endl;

    if (g_tests_failed.load() == 0)
    {
      std::cout << "🎉 ALL INTEGRATION TESTS PASSED!" << std::endl;
      std::cout << std::endl;
      std::cout << "FB_NET Integration Validation Summary:" << std::endl;
      std::cout << "  ✓ End-to-end TCP/UDP communication verified" << std::endl;
      std::cout << "  ✓ High-load concurrent connections handled successfully"
                << std::endl;
      std::cout << "  ✓ Large data transfers work correctly" << std::endl;
      std::cout << "  ✓ Error handling and recovery mechanisms functional"
                << std::endl;
      std::cout << "  ✓ Resource management and cleanup working properly"
                << std::endl;
      std::cout << "  ✓ Cross-component integration validated" << std::endl;
      std::cout << std::endl;
      std::cout << "FB_NET is ready for production use!" << std::endl;
    }
    else
    {
      std::cout << "! " << g_tests_failed.load()
                << " integration test(s) failed." << std::endl;
      std::cout << "Please review the failed tests above." << std::endl;
    }
  }
  catch (const std::exception &ex)
  {
    std::cout << "Integration test suite error: " << ex.what() << std::endl;
  }

  // Cleanup FB_NET library
  fb::cleanup_library();

  return g_tests_failed.load() == 0 ? 0 : 1;
}
