#pragma once

#include <fb/socket_base.h>
#include <fb/socket_address.h>
#include <string>
#include <chrono>

namespace fb {

/**
 * @brief UDP Socket or "Datagram Socket" implementation.
 * This class inherits from socket_base and adds UDP-specific functionality.
 */
class udp_socket : public socket_base
{
public:

  udp_socket();

  explicit udp_socket(socket_address::Family family);
  explicit udp_socket(const socket_address& address, bool reuse_address = true);
  explicit udp_socket(socket_t sockfd);

  udp_socket(const udp_socket&) = delete;
  udp_socket(udp_socket&& other) noexcept;
  udp_socket& operator=(const udp_socket&) = delete;

  udp_socket& operator=(udp_socket&& other) noexcept;

  virtual ~udp_socket() = default;

  int send_to(const void* buffer, int length, const socket_address& address, int flags = 0);
  int receive_from(void* buffer, int length, socket_address& address, int flags = 0);
  int send_to(const std::string& message, const socket_address& address);
  int receive_from(std::string& message, int max_length, socket_address& address);


  void connect(const socket_address& address);
  void disconnect();  ///< Disconnect from connected peer (allows receiving from any source)
  int send_bytes(const void* buffer, int length, int flags = 0);
  int receive_bytes(void* buffer, int length, int flags = 0);
  int send(const std::string& message);
  int receive(std::string& buffer, int max_length);

  void set_broadcast(bool flag);
  bool get_broadcast();
  void set_multicast_ttl(int ttl);
  int  get_multicast_ttl();
  void set_multicast_loopback(bool flag);
  bool get_multicast_loopback();
  void join_group(const socket_address& group_address);
  void join_group(const socket_address& group_address, const socket_address& interface_address);
  void leave_group(const socket_address& group_address);
  void leave_group(const socket_address& group_address, const socket_address& interface_address);

  bool poll_read(const std::chrono::milliseconds& timeout);
  bool poll_write(const std::chrono::milliseconds& timeout);
  int  max_datagram_size() const;
  bool is_connected() const;

private:

  void init_udp_socket(socket_address::Family family);
  void validate_buffer(const void* buffer, int length) const;

  bool m_is_connected; ///< Whether socket is connected to a specific address
};

} // namespace fb
