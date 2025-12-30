#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <ostream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <sys/un.h>
#endif

namespace fb {

/**
 * @brief Address family enumeration for socket addresses
 */
enum class AddressFamily : int
{
  IPv4 = AF_INET,
#ifdef AF_INET6
  IPv6 = AF_INET6,
#endif
#ifdef AF_UNIX
  UNIX_LOCAL = AF_UNIX
#endif
};

/**
 * @brief Represents an internet (IP) endpoint/socket address.
 * The address can belong to IPv4 or IPv6 family.
 * IP addresses consist of a host address and a port number.
 */
class socket_address
{
public:
  using Family = AddressFamily;
  static constexpr Family IPv4 = AddressFamily::IPv4;
#ifdef AF_INET6
  static constexpr Family IPv6 = AddressFamily::IPv6;
#endif

  socket_address();
  explicit socket_address(Family family);
  explicit socket_address(std::uint16_t port);
  explicit socket_address(std::string_view host_and_port);

  socket_address(Family family, std::uint16_t port);
  socket_address(std::string_view host_address, std::uint16_t port_number);
  socket_address(Family family, std::string_view host_address, std::uint16_t port_number);
  socket_address(std::string_view host_address, std::string_view port_number);
  socket_address(Family family, std::string_view host_address, std::string_view port_number);
  socket_address(Family family, std::string_view addr);
  socket_address(const socket_address& other);
  socket_address(socket_address&& other) noexcept;
  socket_address(const struct sockaddr* addr, socklen_t length);

  ~socket_address() = default;

  socket_address& operator=(const socket_address& other);
  socket_address& operator=(socket_address&& other) noexcept;

  std::string host() const;
  std::uint16_t port() const;
  socklen_t length() const;

  const struct sockaddr* addr() const;

  int af() const;

  std::string to_string() const;

  Family family() const;

  bool operator<(const socket_address& other) const;
  bool operator==(const socket_address& other) const;
  bool operator!=(const socket_address& other) const;

  enum : std::size_t
  {
#ifdef AF_INET6
    MAX_ADDRESS_LENGTH = sizeof(struct sockaddr_in6)
#else
    MAX_ADDRESS_LENGTH = sizeof(struct sockaddr_in)
#endif
  };

private:
  void init_ipv4();
  void init_ipv4(std::string_view host, std::uint16_t port);
  void init_ipv4(const struct sockaddr_in* addr);

#ifdef AF_INET6
  void init_ipv6();
  void init_ipv6(std::string_view host, std::uint16_t port);
  void init_ipv6(const struct sockaddr_in6* addr);
#endif

  void parse_host_and_port(std::string_view host_and_port, std::string& host, std::uint16_t& port);
  std::uint16_t parse_port(std::string_view port_str);
  std::uint32_t resolve_host_ipv4(std::string_view host);

#ifdef AF_INET6
  void resolve_host_ipv6(std::string_view host, struct sockaddr_in6& addr);
#endif

  union
  {
    struct sockaddr_in m_ipv4_addr;
#ifdef AF_INET6
    struct sockaddr_in6 m_ipv6_addr;
#endif
    struct sockaddr m_addr;
  };
  
  Family m_family;
};

std::ostream& operator<<(std::ostream& ostr, const socket_address& address);

} // namespace fb
