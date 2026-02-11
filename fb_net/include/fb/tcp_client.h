#pragma once

#include <fb/socket_base.h>
#include <fb/socket_address.h>
#include <fb/fb_signal.hpp>
#include <string>
#include <chrono>

namespace fb {

/**
 * @brief TCP client socket implementation.
 * This class inherits from socket_base and adds TCP-specific client functionality.
 */
class tcp_client : public socket_base
{
public:

  tcp_client();
  explicit tcp_client(socket_address::Family family);
  explicit tcp_client(const socket_address& address);
  tcp_client(const socket_address& address, const std::chrono::milliseconds& timeout);
  explicit tcp_client(socket_t sockfd);
  tcp_client(tcp_client&& other) noexcept;
  
  virtual ~tcp_client() = default;
  tcp_client(const tcp_client&) = delete;
  tcp_client& operator=(const tcp_client&) = delete;

  tcp_client& operator=(tcp_client&& other) noexcept;

  void connect(const socket_address& address);
  void connect(const socket_address& address, const std::chrono::milliseconds& timeout);
  void connect_non_blocking(const socket_address& address);

  int send_bytes(const void* buffer, int length, int flags = 0);
  int receive_bytes(void* buffer, int length, int flags = 0);
  int send(const std::string& message);
  int receive(std::string& buffer, int length);
  int receive_bytes_exact(void* buffer, int length, int flags = 0);
  int send_bytes_all(const void* buffer, int length, int flags = 0);

  void shutdown();
  void shutdown_receive();
  int shutdown_send();
  void send_urgent(unsigned char data);

  // TCP-specific socket options

  void set_no_delay(bool flag);
  bool get_no_delay();
  void set_keep_alive(bool flag);
  bool get_keep_alive();

  // Utility methods

  bool poll_read(const std::chrono::milliseconds& timeout);
  bool poll_write(const std::chrono::milliseconds& timeout);

  // Signals for event-driven programming
  // Note: Signals are emitted on the thread calling the socket method
  // Signal connections/disconnections are thread-safe
  // Use ConnectionType::Queued for cross-thread event handling

  fb::signal<const socket_address&> onConnected;           ///< Emitted when connection is established
  fb::signal<> onDisconnected;                             ///< Emitted when connection is closed
  fb::signal<const std::string&> onConnectionError;        ///< Emitted on connection failure
  fb::signal<const void*, size_t> onDataReceived;          ///< Emitted when data is received
  fb::signal<size_t> onDataSent;                           ///< Emitted when data is sent
  fb::signal<const std::string&> onSendError;              ///< Emitted on send error
  fb::signal<const std::string&> onReceiveError;           ///< Emitted on receive error
  fb::signal<> onShutdownInitiated;                        ///< Emitted when shutdown is called

private:

  void init_tcp_socket(socket_address::Family family);
  int handle_partial_send(const void* buffer, int length, int sent, int flags);
  int handle_partial_receive(void* buffer, int length, int received, int flags);

};

} // namespace fb
