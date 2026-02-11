#pragma once

#include <fb/tcp_client.h>
#include <fb/socket_address.h>
#include <fb/fb_signal.hpp>
#include <memory>
#include <atomic>

namespace fb {

/**
 * @brief Base class for handling individual TCP connections in a multi-threaded server.
 * Serves as the foundation for tcp_server connection handling.
 *
 * Derived classes must implement the run() method to define connection-specific
 * behavior. The connection runs in its own thread and manages the lifecycle
 * of a single client connection.
 */
class tcp_server_connection
{
public:

  tcp_server_connection(tcp_client socket, const socket_address& client_address);
  tcp_server_connection(const tcp_server_connection&) = delete;
  tcp_server_connection(tcp_server_connection&& other) noexcept;

  tcp_server_connection& operator=(const tcp_server_connection&) = delete;
  tcp_server_connection& operator=(tcp_server_connection&& other) noexcept;

  virtual ~tcp_server_connection() = default;
  virtual void run() = 0;

  void start();
  void stop();
  tcp_client& socket();
  const tcp_client& socket() const;
  const socket_address& client_address() const;
  socket_address local_address() const;
  bool is_connected() const;
  bool stop_requested() const;
  std::chrono::steady_clock::duration uptime() const;
  void set_timeout(const std::chrono::milliseconds& timeout);
  void set_no_delay(bool flag);
  void set_keep_alive(bool flag);

  // Signals for connection events
  // Note: Signals are emitted on the connection's worker thread
  fb::signal<> onConnectionStarted;                       ///< Emitted when start() is called
  fb::signal<> onConnectionClosing;                       ///< Emitted before connection closes
  fb::signal<> onConnectionClosed;                        ///< Emitted after connection closes
  fb::signal<const std::exception&> onException;          ///< Emitted when exception occurs

protected:

  virtual void handle_exception(const std::exception& ex) noexcept;
  virtual void on_connection_closing() noexcept;

private:

  tcp_client m_socket;                                ///< Client connection socket
  socket_address m_client_address;                    ///< Client's address
  std::atomic<bool> m_stop_requested;                 ///< Flag indicating stop was requested
  std::chrono::steady_clock::time_point m_start_time; ///< Connection start time

  void validate_socket() const;
};

} // namespace fb
