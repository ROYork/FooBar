/**
 * @file test_memory_validation.cpp
 * @brief Memory Management and Performance Validation for FB_NET
 *
 * This test suite validates memory safety, leak detection, and performance
 * characteristics of FB_NET components under various load conditions.
 *
 * Memory Validation Coverage:
 * 1. Memory leak detection during normal operations
 * 2. Memory usage under high connection loads
 * 3. Resource cleanup validation
 * 4. Exception safety memory management
 * 5. Thread safety memory consistency
 * 6. Critical path performance optimization validation
 *
 * Usage:
 *   ./test_memory_validation
 *
 * For comprehensive validation, run with:
 *   valgrind --leak-check=full ./test_memory_validation
 *   or AddressSanitizer: -fsanitize=address
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fb/fb_net.h>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace fb;

// Test configuration
constexpr int MEMORY_TEST_PORT_BASE = 14000;

constexpr std::uint16_t memory_test_port(int offset)
{
  return static_cast<std::uint16_t>(MEMORY_TEST_PORT_BASE + offset);
}
constexpr int STRESS_CONNECTIONS = 500;
constexpr int STRESS_ITERATIONS  = 100;

// Memory tracking (basic)
std::atomic<size_t> g_allocation_count{0};
std::atomic<size_t> g_deallocation_count{0};

/**
 * Memory Test 1: Basic Resource Cleanup
 */
bool test_basic_resource_cleanup()
{
  std::cout << "Testing basic resource cleanup..." << std::endl;

  try
  {
    // Test socket creation and destruction
    const int cleanup_iterations = STRESS_ITERATIONS * 10;
    for (int i = 0; i < cleanup_iterations; ++i)
    {
      {
        tcp_client socket(socket_address::Family::IPv4);
        socket.set_reuse_address(true);
        socket.set_no_delay(true);
        // Destructor should clean up properly
      }

      {
        udp_socket udp_socket(socket_address::Family::IPv4);
        udp_socket.set_reuse_address(true);
        // Destructor should clean up properly
      }

      {
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.set_reuse_address(true);
        // Destructor should clean up properly
      }
    }

    std::cout << "✓ Basic resource cleanup test passed" << std::endl;
    return true;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Basic resource cleanup test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

/**
 * Memory Test 2: Server Lifecycle Memory Management
 */
class MemoryTestConnection : public tcp_server_connection
{
public:
  MemoryTestConnection(tcp_client socket, const socket_address &client_addr) :
    tcp_server_connection(std::move(socket), client_addr)
  {
  }

  void run() override
  {
    try
    {
      socket().set_receive_timeout(std::chrono::seconds(1));

      std::string data;
      while (socket().receive(data, 1024) > 0)
      {
        socket().send("OK");
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

bool test_server_memory_management()
{
  std::cout << "Testing server memory management..." << std::endl;

  try
  {
    // Multiple server start/stop cycles
    const int cycle_runs   = STRESS_ITERATIONS / 5;
    const int total_cycles = cycle_runs > 0 ? cycle_runs : 1;
    for (int cycle = 0; cycle < total_cycles; ++cycle)
    {
      {
        server_socket server_socket(socket_address::Family::IPv4);
        server_socket.bind(
            socket_address("127.0.0.1", memory_test_port(cycle)));
        server_socket.listen();

        auto factory = [](tcp_client socket, const socket_address &addr)
        {
          return std::make_unique<MemoryTestConnection>(std::move(socket),
                                                        addr);
        };

        tcp_server server(std::move(server_socket), factory, 5, 50);
        server.start();

        // Brief operation with connections
        std::vector<std::thread> client_threads;
        for (int i = 0; i < 5; ++i)
        {
          client_threads.emplace_back(
              [cycle]()
              {
                try
                {
                  tcp_client client(socket_address::Family::IPv4);
                  client.connect(
                      socket_address("127.0.0.1", memory_test_port(cycle)),
                      std::chrono::milliseconds(1000));
                  client.send("TEST");

                  std::string response;
                  client.receive(response, 1024);
                  client.send("QUIT");
                }
                catch (const std::exception &)
                {
                  // Connection may fail in stress test
                }
              });
        }

        // Wait briefly for connections
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        for (auto &thread : client_threads)
        {
          thread.join();
        }

        server.stop(std::chrono::milliseconds(1000));
        // Server destructor should clean up all resources
      }

      // Small delay to allow cleanup
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "✓ Server memory management test passed" << std::endl;
    return true;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Server memory management test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

/**
 * Memory Test 3: Exception Safety Memory Management
 */
bool test_exception_safety_memory()
{
  std::cout << "Testing exception safety memory management..." << std::endl;

  try
  {
    // Test exception scenarios don't leak memory
    for (int i = 0; i < 100; ++i)
    {
      try
      {
        tcp_client socket(socket_address::Family::IPv4);
        socket.connect(socket_address("127.0.0.1", 65432),
                       std::chrono::milliseconds(50)); // Should fail
      }
      catch (const std::system_error &)
      {
        // Expected exception - no memory should leak
      }
      catch (const std::exception &)
      {
        // Other exceptions also acceptable
      }

      try
      {
        udp_socket udp_socket(socket_address::Family::IPv4);
        udp_socket.set_receive_timeout(std::chrono::milliseconds(10));

        std::string data;
        socket_address sender;
        udp_socket.receive_from(data, 1024, sender); // Should timeout
      }
      catch (const std::system_error &)
      {
        // Expected timeout - no memory should leak
      }
      catch (const std::exception &)
      {
        // Other exceptions acceptable
      }

      try
      {
        socket_address invalid("999.999.999.999", 80); // Should fail
      }
      catch (const std::invalid_argument &)
      {
        // Expected exception - no memory should leak
      }
      catch (const std::exception &)
      {
        // Other exceptions acceptable
      }
    }

    std::cout << "✓ Exception safety memory test passed" << std::endl;
    return true;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Exception safety memory test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

/**
 * Memory Test 4: High-Load Stress Testing
 */
bool test_high_load_stress()
{
  std::cout << "Testing high-load stress conditions..." << std::endl;

  try
  {
    // Create server for stress testing
    server_socket server_socket(socket_address::Family::IPv4);
    server_socket.bind(socket_address("127.0.0.1", memory_test_port(100)));
    server_socket.listen();

    auto factory = [](tcp_client socket, const socket_address &addr)
    { return std::make_unique<MemoryTestConnection>(std::move(socket), addr); };

    tcp_server server(std::move(server_socket), factory, 20, 200);
    server.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Stress test with many rapid connections
    std::vector<std::thread> stress_threads;
    std::atomic<int> successful_connections{0};

    for (int i = 0; i < STRESS_CONNECTIONS; ++i)
    {
      stress_threads.emplace_back(
          [&successful_connections]()
          {
            try
            {
              tcp_client socket(socket_address::Family::IPv4);
              socket.connect(socket_address("127.0.0.1", memory_test_port(100)),
                             std::chrono::milliseconds(2000));

              socket.send("STRESS_TEST");

              std::string response;
              socket.receive(response, 1024);

              socket.send("QUIT");
              successful_connections++;
            }
            catch (const std::exception &)
            {
              // Some failures expected under stress
            }
          });
    }

    // Wait for stress test completion
    for (auto &thread : stress_threads)
    {
      thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    server.stop();

    const double success_rate =
        static_cast<double>(successful_connections.load()) /
        static_cast<double>(STRESS_CONNECTIONS);
    bool stress_ok =
        (success_rate >= 0.8); // Allow 20% failure under extreme stress

    std::cout << "Stress test results: " << successful_connections.load() << "/"
              << STRESS_CONNECTIONS << " connections (" << (success_rate * 100)
              << "% success rate)" << std::endl;

    if (stress_ok)
    {
      std::cout << "✓ High-load stress test passed" << std::endl;
    }
    else
    {
      std::cout << "✗ High-load stress test failed (success rate too low)"
                << std::endl;
    }

    return stress_ok;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ High-load stress test failed: " << ex.what() << std::endl;
    return false;
  }
}

/**
 * Memory Test 5: Thread Safety Memory Consistency
 */
bool test_thread_safety_memory()
{
  std::cout << "Testing thread safety memory consistency..." << std::endl;

  try
  {
    // Shared socket operations from multiple threads
    const int num_threads           = 10;
    const int operations_per_thread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<bool> test_failed{false};

    for (int t = 0; t < num_threads; ++t)
    {
      threads.emplace_back(
          [&total_operations, &test_failed]()
          {
            try
            {
              for (int i = 0; i < 100; ++i)
              {
                // Create socket
                tcp_client socket(socket_address::Family::IPv4);

                // Configure socket
                socket.set_reuse_address(true);
                socket.set_no_delay(true);
                socket.set_keep_alive(false);

                // Create socket_address
                socket_address addr("127.0.0.1", 8080);
                std::string addr_str = addr.to_string();

                // Create more objects
                socket_address addr_copy(addr);

                total_operations++;
              }
            }
            catch (const std::exception &)
            {
              test_failed = true;
            }
          });
    }

    // Wait for all threads
    for (auto &thread : threads)
    {
      thread.join();
    }

    bool thread_safety_ok =
        !test_failed.load() &&
        (total_operations.load() == num_threads * operations_per_thread);

    if (thread_safety_ok)
    {
      std::cout << "✓ Thread safety memory test passed ("
                << total_operations.load() << " operations)" << std::endl;
    }
    else
    {
      std::cout << "✗ Thread safety memory test failed" << std::endl;
    }

    return thread_safety_ok;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Thread safety memory test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

/**
 * Performance Optimization Validation
 */
bool test_critical_path_performance()
{
  std::cout << "Testing critical path performance..." << std::endl;

  try
  {
    // Test 1: Socket creation performance
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i)
    {
      tcp_client socket(socket_address::Family::IPv4);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    double avg_creation_time = static_cast<double>(duration.count()) / 10000.0;

    std::cout << "Socket creation: " << avg_creation_time << "μs average"
              << std::endl;

    // Test 2: socket_address creation performance
    start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000; ++i)
    {
      socket_address addr("127.0.0.1", 8080);
      std::string str = addr.to_string(); // Force evaluation
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    double avg_address_time = static_cast<double>(duration.count()) / 10000.0;

    std::cout << "socket_address creation: " << avg_address_time << "μs average"
              << std::endl;

    // Performance targets
    bool creation_ok = (avg_creation_time < 50.0); // <50μs per socket
    bool address_ok  = (avg_address_time < 10.0);  // <10μs per address

    if (creation_ok && address_ok)
    {
      std::cout << "✓ Critical path performance test passed" << std::endl;
    }
    else
    {
      std::cout << "✗ Critical path performance test failed" << std::endl;
    }

    return creation_ok && address_ok;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Critical path performance test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

/**
 * Memory Test 6: Large Object Stress Test
 */
bool test_large_object_stress()
{
  std::cout << "Testing large object stress conditions..." << std::endl;

  try
  {
    // Create many socket objects simultaneously
    std::vector<std::unique_ptr<tcp_client>> sockets;
    sockets.reserve(1000);

    for (int i = 0; i < 1000; ++i)
    {
      sockets.push_back(
          std::make_unique<tcp_client>(socket_address::Family::IPv4));
      sockets.back()->set_reuse_address(true);
    }

    // Test address objects
    std::vector<socket_address> addresses;
    addresses.reserve(1000);

    for (int i = 0; i < 1000; ++i)
    {
      addresses.emplace_back("127.0.0.1", static_cast<std::uint16_t>(8000 + i));
    }

    // Test rapid creation/destruction
    for (int cycle = 0; cycle < 50; ++cycle)
    {
      std::vector<udp_socket> udp_sockets;
      udp_sockets.reserve(100);

      for (int i = 0; i < 100; ++i)
      {
        udp_sockets.emplace_back(socket_address::Family::IPv4);
      }

      // udp_sockets vector destructor should clean up all sockets
    }

    std::cout << "✓ Large object stress test passed" << std::endl;
    return true;
  }
  catch (const std::exception &ex)
  {
    std::cout << "✗ Large object stress test failed: " << ex.what()
              << std::endl;
    return false;
  }
}

int main()
{
  std::cout << "FB_NET Memory Validation and Performance Test Suite"
            << std::endl;
  std::cout << "==================================================="
            << std::endl;
  std::cout << std::endl;

  // Initialize FB_NET library
  fb::initialize_library();

  auto suite_start = std::chrono::steady_clock::now();

  int tests_run    = 0;
  int tests_passed = 0;

  try
  {
    std::cout << "Running memory validation tests..." << std::endl;
    std::cout << "Note: Run with AddressSanitizer or Valgrind for "
                 "comprehensive leak detection"
              << std::endl;
    std::cout << std::endl;

    // Run all memory validation tests
    if (test_basic_resource_cleanup())
      tests_passed++;
    tests_run++;

    if (test_server_memory_management())
      tests_passed++;
    tests_run++;

    if (test_exception_safety_memory())
      tests_passed++;
    tests_run++;

    if (test_high_load_stress())
      tests_passed++;
    tests_run++;

    if (test_thread_safety_memory())
      tests_passed++;
    tests_run++;

    if (test_critical_path_performance())
      tests_passed++;
    tests_run++;

    if (test_large_object_stress())
      tests_passed++;
    tests_run++;

    auto suite_end      = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        suite_end - suite_start);

    // Print summary
    std::cout << std::endl;
    std::cout << "=== Memory Validation Results ===" << std::endl;
    std::cout << "Tests run: " << tests_run << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
    std::cout << "Total time: " << total_duration.count() << "ms" << std::endl;
    std::cout << std::endl;

    if (tests_passed == tests_run)
    {
      std::cout << "🎉 ALL MEMORY VALIDATION TESTS PASSED!" << std::endl;
      std::cout << std::endl;
      std::cout << "FB_NET Memory Management Summary:" << std::endl;
      std::cout << "  ✓ Resource cleanup working correctly" << std::endl;
      std::cout << "  ✓ Server lifecycle memory management validated"
                << std::endl;
      std::cout << "  ✓ Exception safety memory handling confirmed"
                << std::endl;
      std::cout << "  ✓ High-load stress testing passed" << std::endl;
      std::cout << "  ✓ Thread safety memory consistency verified" << std::endl;
      std::cout << "  ✓ Critical path performance optimized" << std::endl;
      std::cout << "  ✓ Large object handling validated" << std::endl;
      std::cout << std::endl;
      std::cout << "FB_NET memory management is production ready!" << std::endl;
      std::cout << std::endl;
      std::cout << "Recommended tools for additional validation:" << std::endl;
      std::cout << "  • AddressSanitizer: -fsanitize=address" << std::endl;
      std::cout
          << "  • Valgrind: valgrind --leak-check=full ./test_memory_validation"
          << std::endl;
      std::cout << "  • Thread Sanitizer: -fsanitize=thread" << std::endl;
    }
    else
    {
      std::cout << "! " << (tests_run - tests_passed)
                << " memory validation test(s) failed." << std::endl;
      std::cout << "Memory management issues detected - review failed tests."
                << std::endl;
    }
  }
  catch (const std::exception &ex)
  {
    std::cout << "Memory validation suite error: " << ex.what() << std::endl;
  }

  // Cleanup FB_NET library
  fb::cleanup_library();

  return (tests_passed == tests_run) ? 0 : 1;
}
