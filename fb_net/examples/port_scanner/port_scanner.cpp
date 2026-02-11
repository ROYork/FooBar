/**
 * @file port_scanner.cpp
 * @brief Port Scanner Example using FB_NET
 * 
 * This example demonstrates concurrent port scanning using FB_NET's
 * tcp_client and poll_set classes. It showcases how to efficiently
 * scan multiple ports simultaneously using polling and non-blocking
 * connection attempts.
 * 
 * Features demonstrated:
 * - Concurrent connection attempts using multiple tcp_client
 * - poll_set usage for managing multiple socket operations
 * - Non-blocking socket operations
 * - Timeout handling for connection attempts
 * - Progress reporting and statistics
 * 
 * Usage:
 *   ./port_scanner <host> <start_port> <end_port> [threads]
 *   Example: ./port_scanner localhost 20 80 10
 */

#include <fb/fb_net.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <iomanip>

using namespace fb;

/**
 * @brief Port scan result
 */
struct ScanResult
{
  int port;
  bool is_open;
  std::string service_info;
  std::chrono::milliseconds response_time;
};

/**
 * @brief Thread-safe results collector
 */
class ResultsCollector
{
  public:
  void add_result(const ScanResult& result)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_results.push_back(result);
    if (result.is_open) {
      m_open_ports++;
    }
    m_completed++;
  }

  void print_progress(int total_ports)
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto completed = m_completed.load();
    const auto open_ports = m_open_ports.load();
    const double percentage =
        total_ports > 0
            ? (static_cast<double>(completed) * 100.0) / static_cast<double>(total_ports)
            : 0.0;
    const auto previous_flags = std::cout.flags();
    const auto previous_precision = std::cout.precision();
    std::cout << "\rProgress: " << std::fixed << std::setprecision(1) << percentage << "% ("
              << completed << "/" << total_ports << "), Open ports: " << open_ports << std::flush;
    std::cout.flags(previous_flags);
    std::cout.precision(previous_precision);
  }

  void print_final_results()
  {
    std::cout << std::endl;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Sort results by port number
    std::sort(m_results.begin(), m_results.end(),
              [](const ScanResult& a, const ScanResult& b) {
                return a.port < b.port;
              });

    std::cout << "\n=== Scan Results ===" << std::endl;
    std::cout << "Open ports found:" << std::endl;

    for (const auto& result : m_results) {
      if (result.is_open) {
        std::cout << "  Port " << std::setw(5) << result.port
                  << " - OPEN (" << result.response_time.count() << "ms)";
        if (!result.service_info.empty()) {
          std::cout << " - " << result.service_info;
        }
        std::cout << std::endl;
      }
    }

    if (m_open_ports == 0) {
      std::cout << "  No open ports found." << std::endl;
    }

    std::cout << "\nSummary:" << std::endl;
    const auto open_ports = static_cast<std::size_t>(m_open_ports.load());
    const auto total_scanned = m_results.size();
    const auto closed_ports =
        total_scanned >= open_ports
            ? total_scanned - open_ports
            : 0U;
    std::cout << "  Total ports scanned: " << total_scanned << std::endl;
    std::cout << "  Open ports: " << open_ports << std::endl;
    std::cout << "  Closed ports: " << closed_ports << std::endl;
  }

  private:
  std::vector<ScanResult> m_results;
  std::mutex m_mutex;
  std::atomic<int> m_completed{0};
  std::atomic<int> m_open_ports{0};
};

/**
 * @brief Port Scanner Worker
 */
class PortScanner
{
  public:

  PortScanner(const std::string& host, int start_port, int end_port,
              int max_concurrent, ResultsCollector& collector):
    m_host(host),
    m_start_port(start_port),
    m_end_port(end_port),
    m_max_concurrent(max_concurrent),
    m_collector(collector),
    m_current_port(start_port),
    m_running(true)
  {
  }

  void run()
  {
    std::cout << "Scanning " << m_host << " ports " << m_start_port
              << "-" << m_end_port << " (max " << m_max_concurrent
              << " concurrent)" << std::endl;

    poll_set poll_set;
    std::vector<std::unique_ptr<ConnectionAttempt>> active_connections;
    auto start_time = std::chrono::steady_clock::now();

    while (m_running && (m_current_port <= m_end_port || !active_connections.empty())) {
      // Start new connections up to the limit
      while (active_connections.size() < static_cast<size_t>(m_max_concurrent) &&
             m_current_port <= m_end_port) {

        try {
          auto attempt = std::make_unique<ConnectionAttempt>(m_host, m_current_port);
          if (attempt->start()) {
            poll_set.add(attempt->socket(), poll_set::POLL_WRITE);
            active_connections.push_back(std::move(attempt));
          }
        } catch (const std::exception&) {
          // Failed to start connection attempt
          ScanResult result;
          result.port = m_current_port;
          result.is_open = false;
          result.response_time = std::chrono::milliseconds(0);
          m_collector.add_result(result);
        }

        m_current_port++;
      }

      if (active_connections.empty()) {
        break; // No more work to do
      }

      // Poll for socket events with timeout
      std::vector<SocketEvent>  ready_sockets;
      if (poll_set.poll(ready_sockets, std::chrono::milliseconds(100)) > 0) {

        // Process ready sockets
        auto it = active_connections.begin();
        while (it != active_connections.end())
        {
          auto& attempt = *it;
          bool completed = false;

          // Check if this socket is ready
          for (const auto& event : ready_sockets)
          {
            if (event.socket_ptr == &attempt->socket())
            {
              completed = attempt->check_completion();
              if (completed)
              {
                poll_set.remove(attempt->socket());
              }
              break;
            }
          }

          // Check for timeout
          if (!completed && attempt->is_timed_out()) {
            attempt->set_result(false, "timeout");
            poll_set.remove(attempt->socket());
            completed = true;
          }

          if (completed) {
            // Collect result
            ScanResult result = attempt->get_result();
            m_collector.add_result(result);

            // Remove from active list
            it = active_connections.erase(it);
          } else {
            ++it;
          }
        }
      }

      // Update progress periodically
      int total_ports = m_end_port - m_start_port + 1;
      m_collector.print_progress(total_ports);
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << std::endl;
    std::cout << "Scan completed in " << duration.count() << "ms" << std::endl;
  }

  void stop()
  {
    m_running = false;
  }

  private:
  /**
     * @brief Single connection attempt tracker
     */
  class ConnectionAttempt
  {
public:
    ConnectionAttempt(const std::string& host, int port)
      : m_host(host), m_port(port), m_start_time(std::chrono::steady_clock::now())
    {
    }

    bool start()
    {
      try {
        m_socket = tcp_client(socket_address::Family::IPv4);
        m_socket.set_blocking(false);
        m_socket.connect(socket_address(m_host, static_cast<std::uint16_t>(m_port)));
        return true;
      } catch (const std::exception&) {
        // Connection failed immediately
        set_result(false, "connection refused");
        return false;
      }
    }

    bool check_completion()
    {
      try {
        // Simple check - if we can write to the socket, connection succeeded
        // This is a simplified approach for the example
        set_result(true, get_service_name());
        return true;
      } catch (const std::exception&) {
        // Connection failed
        set_result(false, "connection failed");
        return true;
      }
    }

    bool is_timed_out() const
    {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time);
      return elapsed.count() > 3000; // 3 second timeout
    }

    void set_result(bool open, const std::string& info)
    {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start_time);

      m_result.port = m_port;
      m_result.is_open = open;
      m_result.service_info = info;
      m_result.response_time = elapsed;
    }

    ScanResult get_result() const
    {
      return m_result;
    }

    tcp_client& socket()
    {
      return m_socket;
    }

private:
    std::string m_host;
    int m_port;
    tcp_client m_socket;
    std::chrono::steady_clock::time_point m_start_time;
    ScanResult m_result;

    std::string get_service_name() const
    {
      // Simple service name lookup
      switch (m_port) {
      case 21: return "FTP";
      case 22: return "SSH";
      case 23: return "Telnet";
      case 25: return "SMTP";
      case 53: return "DNS";
      case 80: return "HTTP";
      case 110: return "POP3";
      case 143: return "IMAP";
      case 443: return "HTTPS";
      case 993: return "IMAPS";
      case 995: return "POP3S";
      default: return "";
      }
    }
  };

  std::string m_host;
  int m_start_port;
  int m_end_port;
  int m_max_concurrent;
  ResultsCollector& m_collector;
  std::atomic<int> m_current_port;
  std::atomic<bool> m_running;
};

int main(int argc, char* argv[])
{
  std::cout << "FB_NET Port Scanner Example" << std::endl;
  std::cout << "============================" << std::endl;

  // Initialize FB_NET library
  fb::initialize_library();

  try {
    // Parse command line arguments
    if (argc < 4) {
      std::cerr << "Usage: " << argv[0] << " <host> <start_port> <end_port> [max_concurrent]" << std::endl;
      std::cerr << "Example: " << argv[0] << " localhost 20 80 20" << std::endl;
      return 1;
    }

    std::string host = argv[1];
    int start_port = std::atoi(argv[2]);
    int end_port = std::atoi(argv[3]);
    int max_concurrent = (argc > 4) ? std::atoi(argv[4]) : 50;

    // Validate arguments
    if (start_port < 1 || start_port > 65535 || end_port < 1 || end_port > 65535) {
      std::cerr << "Invalid port range (1-65535)" << std::endl;
      return 1;
    }
    if (start_port > end_port) {
      std::cerr << "Start port must be <= end port" << std::endl;
      return 1;
    }
    if (max_concurrent < 1 || max_concurrent > 1000) {
      std::cerr << "Invalid max concurrent connections (1-1000)" << std::endl;
      return 1;
    }

    // Create results collector
    ResultsCollector collector;

    // Create and run scanner
    PortScanner scanner(host, start_port, end_port, max_concurrent, collector);
    scanner.run();

    // Print final results
    collector.print_final_results();

  } catch (const std::exception& ex) {
    std::cerr << "Scanner error: " << ex.what() << std::endl;
    return 1;
  }

  // Cleanup FB_NET library
  fb::cleanup_library();
  return 0;
}
