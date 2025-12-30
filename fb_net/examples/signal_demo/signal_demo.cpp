/**
 * @file signal_demo.cpp
 * @brief Demonstration of Signal Integration with FB_NET
 *
 * This example demonstrates how signals enable event-driven network programming
 * with fb_net components. It shows:
 * 1. Connection monitoring with signals
 * 2. Real-time statistics collection
 * 3. Event-driven logging
 * 4. Decoupled application architecture
 */

#include <atomic>
#include <chrono>
#include <fb/fb_net.h>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace fb;

/**
 * @brief Connection monitor that tracks network activity via signals
 */
class ConnectionMonitor
{
public:
  ConnectionMonitor(tcp_client & client, const std::string & name) :
    m_name(name),
    client_(client),
    connected_time_()
  {
    std::cout << "\n[MONITOR] Attaching to " << m_name << std::endl;

    // Connect to lifecycle signals
    client_.onConnected.connect(
      [this](const socket_address & addr)
      {
        std::cout << "[" << m_name << "] Connected to " << addr.to_string() << std::endl;
        connected_time_ = std::chrono::steady_clock::now();
      });

    client_.onDisconnected.connect(
      [this]()
      {
        auto duration = std::chrono::steady_clock::now() - connected_time_;
        auto seconds  = std::chrono::duration_cast<std::chrono::seconds>(duration);
        std::cout << "[" << m_name << "] Disconnected after " << seconds.count() << "s" << std::endl;
        print_statistics();
      });

    // Monitor data transfer
    client_.onDataReceived.connect(
      [this](const void *, size_t len)
      {
        bytes_received_ += len;
        ++packets_received_;
      });

    client_.onDataSent.connect(
      [this](size_t len)
      {
        bytes_sent_ += len;
        ++packets_sent_;
      });

    // Monitor errors
    client_.onConnectionError.connect(
      [this](const std::string & error)
      {
        std::cerr << "[" << m_name << "] ERROR: " << error << std::endl;
        ++error_count_;
      });

    client_.onSendError.connect(
      [this](const std::string & error)
      {
        std::cerr << "[" << m_name << "] SEND ERROR: " << error << std::endl;
        ++error_count_;
      });

    client_.onReceiveError.connect(
      [this](const std::string & error)
      {
        std::cerr << "[" << m_name << "] RECEIVE ERROR: " << error << std::endl;
        ++error_count_;
      });

    // Monitor shutdown
    client_.onShutdownInitiated.connect(
      [this]() { std::cout << "[" << m_name << "] Shutdown initiated" << std::endl; });
  }

  void print_statistics() const
  {
    std::cout << "\n=== Statistics for " << m_name << " ===" << std::endl;
    std::cout << "  Packets RX:   " << packets_received_ << std::endl;
    std::cout << "  Packets TX:   " << packets_sent_ << std::endl;
    std::cout << "  Bytes RX:     " << format_bytes(bytes_received_) << std::endl;
    std::cout << "  Bytes TX:     " << format_bytes(bytes_sent_) << std::endl;
    std::cout << "  Errors:       " << error_count_ << std::endl;
    std::cout << "================================" << std::endl;
  }

  uint64_t bytes_received() const { return bytes_received_; }
  uint64_t bytes_sent() const { return bytes_sent_; }

private:

  std::string m_name;
  tcp_client & client_;
  std::chrono::steady_clock::time_point connected_time_;

  std::atomic<uint64_t> bytes_received_{0};
  std::atomic<uint64_t> bytes_sent_{0};
  std::atomic<uint32_t> packets_received_{0};
  std::atomic<uint32_t> packets_sent_{0};
  std::atomic<uint32_t> error_count_{0};

  static std::string format_bytes(uint64_t bytes)
  {
    std::ostringstream oss;
    if (bytes < 1024)
    {
      oss << bytes << " B";
    }
    else if (bytes < 1024 * 1024)
    {
      oss << std::fixed << std::setprecision(2)
          << (static_cast<double>(bytes) / 1024.0) << " KB";
    }
    else
    {
      oss << std::fixed << std::setprecision(2)
          << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB";
    }
    return oss.str();
  }
};

/**
 * @brief Example 1: Simple HTTP Client with Signal Monitoring
 */
void example1_http_client_with_monitoring()
{
  std::cout << "\n" << std::string(80, '=') << std::endl;
  std::cout << "EXAMPLE 1: HTTP Client with Signal-Based Monitoring" << std::endl;
  std::cout << std::string(80, '=') << std::endl;

  try
  {
    tcp_client client;

    // Attach monitor - automatically tracks all events
    ConnectionMonitor monitor(client, "HTTP-Client");

    // Additional custom logging
    client.onDataReceived.connect(
      [](const void *, size_t len)
      { std::cout << "[DATA] Received " << len << " bytes" << std::endl; });

    // Connect and send HTTP request
    std::cout << "\nConnecting to httpbin.org:80..." << std::endl;
    client.connect(socket_address("httpbin.org", 80));

    std::string request = "GET /get HTTP/1.1\r\n"
                          "Host: httpbin.org\r\n"
                          "Connection: close\r\n\r\n";

    std::cout << "Sending HTTP GET request..." << std::endl;
    client.send(request);

    // Receive response
    std::cout << "\nReceiving response..." << std::endl;
    std::string response;
    while (client.receive(response, 4096) > 0)
    {
      // Data reception automatically triggers onDataReceived signal
    }

    // Connection will close, triggering onDisconnected with statistics
  }
  catch (const std::exception & ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}

/**
 * @brief Example 2: Multiple concurrent connections with monitoring
 */
void example2_multiple_connections()
{
  std::cout << "\n" << std::string(80, '=') << std::endl;
  std::cout << "EXAMPLE 2: Multiple Concurrent Connections" << std::endl;
  std::cout << std::string(80, '=') << std::endl;

  try
  {
    // Create multiple clients
    tcp_client client1;
    tcp_client client2;

    // Attach monitors to both
    ConnectionMonitor monitor1(client1, "Client-1");
    ConnectionMonitor monitor2(client2, "Client-2");

    // Shared signal for aggregated statistics
    std::atomic<uint64_t> total_bytes_received{0};

    client1.onDataReceived.connect([&total_bytes_received](const void *, size_t len)
                                   { total_bytes_received += len; });

    client2.onDataReceived.connect([&total_bytes_received](const void *, size_t len)
                                   { total_bytes_received += len; });

    std::cout << "\nConnecting both clients..." << std::endl;

    // Connect first client
    client1.connect(socket_address("httpbin.org", 80));
    std::string request1
      = "GET /bytes/1024 HTTP/1.1\r\nHost: httpbin.org\r\nConnection: close\r\n\r\n";
    client1.send(request1);

    // Connect second client
    client2.connect(socket_address("httpbin.org", 80));
    std::string request2
      = "GET /bytes/2048 HTTP/1.1\r\nHost: httpbin.org\r\nConnection: close\r\n\r\n";
    client2.send(request2);

    // Receive from both
    std::string response;
    while (client1.receive(response, 4096) > 0)
    {
    }
    while (client2.receive(response, 4096) > 0)
    {
    }

    std::cout << "\n[AGGREGATE] Total bytes received across all connections: "
              << total_bytes_received << std::endl;
  }
  catch (const std::exception & ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}

/**
 * @brief Example 3: Event-driven protocol handler
 */
class ProtocolStateMachine
{
public:
  enum State
  {
    DISCONNECTED,
    CONNECTED,
    SENDING,
    RECEIVING,
    DONE
  };

  ProtocolStateMachine(tcp_client & client) :
    client_(client),
    state_(DISCONNECTED)
  {
    // Connect signals to drive state machine
    client_.onConnected.connect(
      [this](const socket_address &)
      {
        std::cout << "[STATE] DISCONNECTED -> CONNECTED" << std::endl;
        state_ = CONNECTED;
        send_request();
      });

    client_.onDataSent.connect(
      [this](size_t)
      {
        if (state_ == SENDING)
        {
          std::cout << "[STATE] SENDING -> RECEIVING" << std::endl;
          state_ = RECEIVING;
        }
      });

    client_.onDataReceived.connect([this](const void * data, size_t len)
                                   { response_data_.append(static_cast<const char *>(data), len); });

    client_.onDisconnected.connect(
      [this]()
      {
        std::cout << "[STATE] -> DONE" << std::endl;
        state_ = DONE;
        std::cout << "[RESPONSE] Received " << response_data_.size() << " bytes" << std::endl;
      });
  }

  void send_request()
  {
    std::cout << "[STATE] CONNECTED -> SENDING" << std::endl;
    state_              = SENDING;

    std::string request = "GET /get HTTP/1.1\r\n"
                          "Host: httpbin.org\r\n"
                          "Connection: close\r\n\r\n";

    client_.send(request);
  }

  State state() const { return state_; }

private:
  tcp_client & client_;
  State state_;
  std::string response_data_;
};

void example3_state_machine()
{
  std::cout << "\n" << std::string(80, '=') << std::endl;
  std::cout << "EXAMPLE 3: Event-Driven Protocol State Machine" << std::endl;
  std::cout << std::string(80, '=') << std::endl;

  try
  {
    tcp_client client;
    ProtocolStateMachine state_machine(client);

    std::cout << "\nConnecting to httpbin.org..." << std::endl;
    client.connect(socket_address("httpbin.org", 80));

    // State machine automatically handles the protocol
    std::string response;
    while (client.receive(response, 4096) > 0)
    {
      // Reception triggers state transitions
    }

    std::cout << "\nProtocol completed successfully!" << std::endl;
  }
  catch (const std::exception & ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
}

/**
 * @brief Main entry point
 */
int main()
{
  std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║              FB_NET SIGNAL INTEGRATION DEMONSTRATION                         ║
║                                                                              ║
║  This demo shows how signals enable event-driven network programming         ║
║  with automatic monitoring, statistics, and decoupled architecture.          ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;

  // Initialize FB_NET library
  fb::initialize_library();

  try
  {
    // Run examples
    example1_http_client_with_monitoring();

    std::cout << "\nPress Enter to continue to next example..." << std::endl;
    std::cin.get();

    example2_multiple_connections();

    std::cout << "\nPress Enter to continue to next example..." << std::endl;
    std::cin.get();

    example3_state_machine();

    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "DEMONSTRATION COMPLETE" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    std::cout << R"(
Key Benefits Demonstrated:
✓ Automatic connection monitoring
✓ Real-time statistics collection
✓ Event-driven state machines
✓ Decoupled architecture
✓ Zero-cost abstraction when not used

Next Steps:
- Check the analysis report in signal_stage/fb_signal/doc/
- Explore tcp_server signal integration
- Build custom signal-driven applications
)" << std::endl;
  }
  catch (const std::exception & ex)
  {
    std::cerr << "Fatal error: " << ex.what() << std::endl;
    return 1;
  }

  // Cleanup FB_NET library
  fb::cleanup_library();
  return 0;
}
