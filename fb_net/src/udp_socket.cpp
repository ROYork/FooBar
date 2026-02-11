#include <fb/udp_socket.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace fb
{

/**
 * @brief Default constructor creates an uninitialized UDP socket.
 * @sa init_udp_socket
 */
udp_socket::udp_socket() :
  socket_base(),
  m_is_connected(false)
{
  init_udp_socket(socket_address::IPv4);
}

/**
 * @brief Create a UDP socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
udp_socket::udp_socket(socket_address::Family family) :
  socket_base(),
  m_is_connected(false)
{
  init_udp_socket(family);
}

/**
 * @brief Create a UDP socket and bind to local address
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR
 * @throws std::system_error, std::invalid_argument
 */
udp_socket::udp_socket(const socket_address &address, bool reuse_address) :
  socket_base(),
  m_is_connected(false)
{
  init_udp_socket(address.family());
  bind(address, reuse_address);
}

/**
 * @brief Create udp_socket from existing socket descriptor
 * @param sockfd Socket descriptor to wrap
 */
udp_socket::udp_socket(socket_t sockfd) :
  socket_base(sockfd),
  m_is_connected(false)
{
}

/**
 * @brief Move constructor
 */
udp_socket::udp_socket(udp_socket &&other) noexcept :
  socket_base(std::move(other)),
  m_is_connected(other.m_is_connected)
{
  other.m_is_connected = false;
}

/**
 * @brief Move assignment operator
 */
udp_socket &udp_socket::operator=(udp_socket &&other) noexcept
{
  socket_base::operator=(std::move(other));
  m_is_connected       = other.m_is_connected;
  other.m_is_connected = false;
  return *this;
}

/**
 * @brief Send datagram to specific address
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param address Destination address
 * @param flags Send flags (default 0)
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_socket::send_to(const void *buffer,
                        int length,
                        const socket_address &address,
                        int flags)
{
  check_initialized();
  validate_buffer(buffer, length);

  if (length == 0)
  {
    return 0;
  }

  int sent = ::sendto(sockfd(), static_cast<const char *>(buffer), length,
                      flags, address.addr(), address.length());

  if (sent < 0)
  {
    error("Failed to send datagram");
  }

  return sent;
}

/**
 * @brief Receive datagram from any source
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @param address Output parameter for source address
 * @param flags Receive flags (default 0)
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_socket::receive_from(void *buffer,
                             int length,
                             socket_address &address,
                             int flags)
{
  check_initialized();
  validate_buffer(buffer, length);

  if (length == 0)
  {
    return 0;
  }

  sockaddr_storage addr_storage;
  socklen_t addr_len = sizeof(addr_storage);

  int received =
      ::recvfrom(sockfd(), static_cast<char *>(buffer), length, flags,
                 reinterpret_cast<sockaddr *>(&addr_storage), &addr_len);

  if (received < 0)
  {
    error("Failed to receive datagram");
  }

  // Convert sockaddr to socket_address
  address =
      socket_address(reinterpret_cast<sockaddr *>(&addr_storage), addr_len);

  return received;
}

/**
 * @brief Send string datagram to specific address
 * @param message String to send
 * @param address Destination address
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_socket::send_to(const std::string &message,
                        const socket_address &address)
{
  if (message.empty())
  {
    return 0;
  }
  return send_to(message.data(), static_cast<int>(message.size()), address);
}

/**
 * @brief Receive string datagram from any source
 * @param message String to store received data
 * @param max_length Maximum number of bytes to receive
 * @param address Output parameter for source address
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_socket::receive_from(std::string &message,
                             int max_length,
                             socket_address &address)
{
  if (max_length <= 0)
  {
    return 0;
  }

  message.resize(max_length);
  int received = receive_from(&message[0], max_length, address);
  if (received >= 0)
  {
    message.resize(received);
  }
  else
  {
    message.clear();
  }
  return received;
}

/**
 * @brief Connect UDP socket to remote address (sets default destination)
 * @param address Remote address to connect to
 * @throws std::system_error
 * @note This doesn't create a TCP-style connection, just sets default
 * destination
 */
void udp_socket::connect(const socket_address &address)
{
  try
  {
    socket_base::connect(address);
    m_is_connected = true;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Disconnect from connected peer
 *
 * This removes the peer address filter so the socket can receive from any source.
 * On POSIX systems, this is done by connecting to AF_UNSPEC.
 */
void udp_socket::disconnect()
{
  if (!m_is_connected)
  {
    return;  // Already disconnected
  }

  int result = 0;
#ifdef _WIN32
  sockaddr_in unspec_addr{};
  unspec_addr.sin_family = AF_UNSPEC;
  result = ::connect(sockfd(), reinterpret_cast<sockaddr*>(&unspec_addr), sizeof(unspec_addr));
#else
  sockaddr unspec_addr{};
  unspec_addr.sa_family = AF_UNSPEC;
  result = ::connect(sockfd(), &unspec_addr, sizeof(unspec_addr));
#endif

  if (result != 0)
  {
    // macOS/BSD returns EAFNOSUPPORT for AF_UNSPEC connect on UDP sockets,
    // but the peer disassociation succeeds regardless
    int err = last_error();
    if (err != EAFNOSUPPORT)
    {
      error("disconnect");
    }
  }

  m_is_connected = false;
  set_connected(false);
}

/**
 * @brief Send data to connected address
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param flags Send flags (default 0)
 * @return Number of bytes actually sent
 * @throws std::system_error
 * @note Socket must be connected first
 */
int udp_socket::send_bytes(const void *buffer, int length, int flags)
{
  check_initialized();
  if (!m_is_connected)
  {
    throw std::logic_error("Socket not connected - use send_to instead");
  }
  validate_buffer(buffer, length);

  if (length == 0)
  {
    return 0;
  }

  int sent = ::send(sockfd(), static_cast<const char *>(buffer), length, flags);

  if (sent < 0)
  {
    error("Failed to send data");
  }

  return sent;
}

/**
 * @brief Receive data from connected address
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @param flags Receive flags (default 0)
 * @return Number of bytes actually received
 * @throws std::system_error
 * @note Socket must be connected first
 */
int udp_socket::receive_bytes(void *buffer, int length, int flags)
{
  check_initialized();

  if (!m_is_connected)
  {
    throw std::logic_error("Socket not connected - use receive_from instead");
  }
  validate_buffer(buffer, length);

  if (length == 0)
  {
    return 0;
  }

  int received = ::recv(sockfd(), static_cast<char *>(buffer), length, flags);

  if (received < 0)
  {
    error("Failed to receive data");
  }

  return received;
}

/**
 * @brief Send string data to connected address
 * @param message String to send
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_socket::send(const std::string &message)
{
  if (message.empty())
  {
    return 0;
  }
  return send_bytes(message.data(), static_cast<int>(message.size()));
}

/**
 * @brief Receive string data from connected address
 * @param buffer Buffer to store received data
 * @param max_length Maximum number of bytes to receive
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_socket::receive(std::string &buffer, int max_length)
{
  if (max_length <= 0)
  {
    return 0;
  }

  buffer.resize(max_length);
  int received = receive_bytes(&buffer[0], max_length);
  if (received >= 0)
  {
    buffer.resize(received);
  }
  else
  {
    buffer.clear();
  }
  return received;
}

// UDP-specific socket options

/**
 * @brief Set broadcast option
 * @param flag Enable/disable broadcast capability
 * @throws std::system_error
 */
void udp_socket::set_broadcast(bool flag) { socket_base::set_broadcast(flag); }

/**
 * @brief Get broadcast option
 * @return True if broadcast is enabled
 * @throws std::system_error
 */
bool udp_socket::get_broadcast() { return socket_base::get_broadcast(); }

/**
 * @brief Set multicast time-to-live (TTL)
 * @param ttl TTL value (1-255)
 * @throws std::system_error
 */
void udp_socket::set_multicast_ttl(int ttl)
{
  check_initialized();
  if (ttl < 1 || ttl > 255)
  {
    throw std::out_of_range("Invalid TTL value (must be 1-255)");
  }

#ifdef _WIN32
  DWORD value = static_cast<DWORD>(ttl);
  if (::setsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_TTL,
                   reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
  {
    error("Failed to set multicast TTL");
  }
#else
  if (::setsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) !=
      0)
  {
    error("Failed to set multicast TTL");
  }
#endif
}

/**
 * @brief Get multicast time-to-live (TTL)
 * @return Current TTL value
 * @throws std::system_error
 */
int udp_socket::get_multicast_ttl()
{
  check_initialized();

#ifdef _WIN32
  DWORD value;
  socklen_t len = sizeof(value);
  if (::getsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_TTL,
                   reinterpret_cast<char *>(&value), &len) != 0)
  {
    error("Failed to get multicast TTL");
  }
  return static_cast<int>(value);
#else
  int value;
  socklen_t len = sizeof(value);
  if (::getsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_TTL, &value, &len) != 0)
  {
    error("Failed to get multicast TTL");
  }
  return value;
#endif
}

/**
 * @brief Set multicast loopback option
 * @param flag Enable/disable loopback
 * @throws std::system_error
 */
void udp_socket::set_multicast_loopback(bool flag)
{
  check_initialized();

#ifdef _WIN32
  DWORD value = flag ? 1 : 0;
  if (::setsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_LOOP,
                   reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
  {
    error("Failed to set multicast loopback");
  }
#else
  int value = flag ? 1 : 0;
  if (::setsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_LOOP, &value,
                   sizeof(value)) != 0)
  {
    error("Failed to set multicast loopback");
  }
#endif
}

/**
 * @brief Get multicast loopback option
 * @return True if loopback is enabled
 * @throws std::system_error
 */
bool udp_socket::get_multicast_loopback()
{
  check_initialized();

#ifdef _WIN32
  DWORD value;
  socklen_t len = sizeof(value);
  if (::getsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_LOOP,
                   reinterpret_cast<char *>(&value), &len) != 0)
  {
    error("Failed to get multicast loopback");
  }
  return value != 0;
#else
  int value;
  socklen_t len = sizeof(value);
  if (::getsockopt(sockfd(), IPPROTO_IP, IP_MULTICAST_LOOP, &value, &len) != 0)
  {
    error("Failed to get multicast loopback");
  }
  return value != 0;
#endif
}

/**
 * @brief Join multicast group
 * @param group_address Multicast group address
 * @throws std::system_error
 */
void udp_socket::join_group(const socket_address &group_address)
{
  check_initialized();

  // Use INADDR_ANY for interface (let system choose)
  socket_address any_interface("0.0.0.0", 0);
  join_group(group_address, any_interface);
}

/**
 * @brief Join multicast group on specific interface
 * @param group_address Multicast group address
 * @param interface_address Local interface address
 * @throws std::system_error
 */
void udp_socket::join_group(const socket_address &group_address,
                            const socket_address &interface_address)
{
  check_initialized();

  if (group_address.family() != interface_address.family())
  {
    throw std::invalid_argument(
        "Group and interface addresses must have same family");
  }

  if (group_address.family() == socket_address::IPv4)
  {
    struct ip_mreq mreq;
    std::memset(&mreq, 0, sizeof(mreq));

    // Set multicast group address
    struct sockaddr_in *group_addr = reinterpret_cast<struct sockaddr_in *>(
        const_cast<sockaddr *>(group_address.addr()));
    mreq.imr_multiaddr = group_addr->sin_addr;

    // Set interface address
    struct sockaddr_in *iface_addr = reinterpret_cast<struct sockaddr_in *>(
        const_cast<sockaddr *>(interface_address.addr()));
    mreq.imr_interface = iface_addr->sin_addr;

    if (::setsockopt(sockfd(), IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     reinterpret_cast<const char *>(&mreq), sizeof(mreq)) != 0)
    {
      error("Failed to join multicast group");
    }
  }
  else
  {
    throw std::logic_error("IPv6 multicast not yet implemented");
  }
}

/**
 * @brief Leave multicast group
 * @param group_address Multicast group address
 * @throws std::system_error
 */
void udp_socket::leave_group(const socket_address &group_address)
{
  check_initialized();

  // Use INADDR_ANY for interface (let system choose)
  socket_address any_interface("0.0.0.0", 0);
  leave_group(group_address, any_interface);
}

/**
 * @brief Leave multicast group on specific interface
 * @param group_address Multicast group address
 * @param interface_address Local interface address
 * @throws std::system_error
 */
void udp_socket::leave_group(const socket_address &group_address,
                             const socket_address &interface_address)
{
  check_initialized();

  if (group_address.family() != interface_address.family())
  {
    throw std::invalid_argument(
        "Group and interface addresses must have same family");
  }

  if (group_address.family() == socket_address::IPv4)
  {
    struct ip_mreq mreq;
    std::memset(&mreq, 0, sizeof(mreq));

    // Set multicast group address
    struct sockaddr_in *group_addr = reinterpret_cast<struct sockaddr_in *>(
        const_cast<sockaddr *>(group_address.addr()));
    mreq.imr_multiaddr = group_addr->sin_addr;

    // Set interface address
    struct sockaddr_in *iface_addr = reinterpret_cast<struct sockaddr_in *>(
        const_cast<sockaddr *>(interface_address.addr()));
    mreq.imr_interface = iface_addr->sin_addr;

    if (::setsockopt(sockfd(), IPPROTO_IP, IP_DROP_MEMBERSHIP,
                     reinterpret_cast<const char *>(&mreq), sizeof(mreq)) != 0)
    {
      error("Failed to leave multicast group");
    }
  }
  else
  {
    throw std::logic_error("IPv6 multicast not yet implemented");
  }
}

// Utility methods

/**
 * @brief Check if there's data available to read
 * @param timeout Poll timeout
 * @return True if data is available
 * @throws std::system_error
 */
bool udp_socket::poll_read(const std::chrono::milliseconds &timeout)
{
  return poll(timeout, socket_base::SELECT_READ);
}

/**
 * @brief Check if socket is ready for writing
 * @param timeout Poll timeout
 * @return True if socket is ready for writing
 * @throws std::system_error
 */
bool udp_socket::poll_write(const std::chrono::milliseconds &timeout)
{
  return poll(timeout, socket_base::SELECT_WRITE);
}

/**
 * @brief Get maximum datagram size for this socket
 * @return Maximum datagram size in bytes
 */
int udp_socket::max_datagram_size() const
{
  // Theoretical maximum UDP datagram size
  // In practice, this is limited by network MTU
  return 65507; // 65535 (IP max) - 20 (IP header) - 8 (UDP header)
}

/**
 * @brief Check if socket is connected (has default destination)
 * @return True if socket is connected
 */
bool udp_socket::is_connected() const
{
  return m_is_connected && socket_base::is_connected();
}

// Private helper methods

/**
 * @brief Initialize UDP socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
void udp_socket::init_udp_socket(socket_address::Family family)
{
  try
  {
    init(family, DATAGRAM_SOCKET);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Validate buffer parameters
 * @param buffer Buffer pointer
 * @param length Buffer length
 * @throws std::system_error if invalid
 */
void udp_socket::validate_buffer(const void *buffer, int length) const
{
  if (!buffer && length > 0)
  {
    throw std::invalid_argument("Buffer cannot be null for non-zero length");
  }
  if (length < 0)
  {
    throw std::invalid_argument("Length cannot be negative");
  }
  if (length > max_datagram_size())
  {
    throw std::length_error("Datagram size exceeds maximum allowed size");
  }
}

} // namespace fb
