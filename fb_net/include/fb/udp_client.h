#pragma once

#include <fb/udp_socket.h>
#include <fb/socket_address.h>
#include <fb/fb_signal.hpp>
#include <string>
#include <chrono>

namespace fb {

/**
 * @class fb::udp_client
 * @brief High-level UDP Client implementation.
 *
 * Provides a simplified interface for UDP communication, built on top
 * of udp_socket. It supports both connected and connectionless operations,
 * automatic addressing, timeouts, and broadcast/multicast operations.
 */
class udp_client
{
public:

  udp_client();
  explicit udp_client(socket_address::Family family);
  explicit udp_client(const socket_address& local_address, bool reuse_address = true);
  udp_client(const socket_address& remote_address, const socket_address& local_address = socket_address());
  udp_client(const udp_client&) = delete;
  udp_client(udp_client&& other) noexcept;
  udp_client& operator=(const udp_client&) = delete;
  udp_client& operator=(udp_client&& other) noexcept;
  ~udp_client();

  // Connection management

  void connect(const socket_address& address);
  void bind(const socket_address& address, bool reuse_address = true);
  void disconnect();
  void close();

  // Connected UDP operations (when connected to a specific address)

  int send(const void* buffer, std::size_t length);
  int send(const std::string& message);
  int receive(void* buffer, std::size_t length);
  int receive(std::string& message, std::size_t max_length = 1024);

  // Connectionless UDP operations (explicit addressing)

  int send_to(const void* buffer, std::size_t length, const socket_address& address);
  int send_to(const std::string& message, const socket_address& address);
  int receive_from(void* buffer, std::size_t length, socket_address& sender_address);
  int receive_from(std::string& message, std::size_t max_length, socket_address& sender_address);
  int send_with_timeout(const void* buffer,
                        std::size_t length,
                        const std::chrono::milliseconds& timeout);

  int receive_with_timeout(void* buffer,
                           std::size_t length,
                           const std::chrono::milliseconds& timeout);


  void set_broadcast(bool flag);
  bool get_broadcast();
  int broadcast(const void* buffer, std::size_t length, std::uint16_t port);
  int broadcast(const std::string& message, std::uint16_t port);

  void set_timeout(const std::chrono::milliseconds& timeout);
  std::chrono::milliseconds get_timeout();

  void set_receive_buffer_size(int size);
  int get_receive_buffer_size();
  void set_send_buffer_size(int size);
  int get_send_buffer_size();

  // Status queries

  bool is_connected() const;
  bool is_closed() const;

  socket_address local_address() const;
  socket_address remote_address() const;

  bool has_data_available(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));

  udp_socket& socket();

  const udp_socket& socket() const;

  // Signal members for event-driven programming
  // Connection lifecycle signals
  fb::signal<const socket_address&> onConnected;       ///< Emitted when connected to remote address
  fb::signal<> onDisconnected;                         ///< Emitted when disconnected

  // Data transfer signals
  fb::signal<const void*, std::size_t> onDataSent;     ///< Emitted after successful send
  fb::signal<const void*, std::size_t> onDataReceived; ///< Emitted after successful receive
  fb::signal<const void*, std::size_t, const socket_address&> onDatagramReceived; ///< Emitted on datagram reception with sender

  // Broadcast/Multicast signals
  fb::signal<const void*, std::size_t> onBroadcastSent;    ///< Emitted after successful broadcast send
  fb::signal<const void*, std::size_t> onBroadcastReceived;///< Emitted after receiving broadcast

  // Error signals
  fb::signal<const std::string&> onSendError;    ///< Emitted on send failure
  fb::signal<const std::string&> onReceiveError; ///< Emitted on receive failure
  fb::signal<const std::string&> onTimeoutError; ///< Emitted on timeout

protected:

  void validate_socket() const;
  void validate_buffer(const void* buffer, std::size_t length) const;

private:

    udp_socket m_socket; ///< Underlying datagram socket
    bool m_is_connected; ///< Whether client is connected to specific address
    socket_address m_remote_address; ///< Remote address (when connected)
    std::chrono::milliseconds m_timeout; ///< Default timeout for operations

    static constexpr auto DEFAULT_TIMEOUT = std::chrono::milliseconds(30000);
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 65507; // Max UDP payload size

    void init_client(socket_address::Family family);
};

} // namespace fb
