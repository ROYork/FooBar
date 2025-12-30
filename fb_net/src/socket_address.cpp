#include <fb/socket_address.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace
{

std::string format_gai_error(std::string_view host, int error_code)
{
#if defined(_WIN32)
  const char *error_text = gai_strerrorA(error_code);
#else
  const char *error_text = gai_strerror(error_code);
#endif

  std::string message = "Failed to resolve hostname: ";
  message.append(host);

  if (error_text != nullptr)
  {
    message.append(" (");
    message.append(error_text);
    message.push_back(')');
  }
  else
  {
    message.append(" (code: ");
    message.append(std::to_string(error_code));
    message.push_back(')');
  }

  return message;
}

} // namespace

namespace fb
{

socket_address::socket_address() :
  m_family(AddressFamily::IPv4)
{
  init_ipv4();
}

socket_address::socket_address(Family family) :
  m_family(family)
{
  switch (family)
  {
  case AddressFamily::IPv4:
    init_ipv4();
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    init_ipv6();
    break;
#endif
  default:
    throw std::invalid_argument("Unsupported address family");
  }
}

socket_address::socket_address(std::uint16_t port) :
  m_family(AddressFamily::IPv4)
{
  init_ipv4("0.0.0.0", port);
}

socket_address::socket_address(Family family, std::uint16_t port) :
  m_family(family)
{
  switch (family)
  {
  case AddressFamily::IPv4:
    init_ipv4("0.0.0.0", port);
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    init_ipv6("::", port);
    break;
#endif
  default:
    throw std::invalid_argument("Unsupported address family");
  }
}

socket_address::socket_address(std::string_view host_address,
                               std::uint16_t port_number) :
  m_family(AddressFamily::IPv4)
{
  // Try IPv4 first
  if (host_address.find(':') == std::string_view::npos)
  {
    init_ipv4(host_address, port_number);
  }
  else
  {
#ifdef AF_INET6
    m_family = AddressFamily::IPv6;
    init_ipv6(host_address, port_number);
#else
    throw std::invalid_argument("IPv6 not supported on this platform");
#endif
  }
}

socket_address::socket_address(Family family,
                               std::string_view host_address,
                               std::uint16_t port_number) :
  m_family(family)
{
  switch (family)
  {
  case AddressFamily::IPv4:
    init_ipv4(host_address, port_number);
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    init_ipv6(host_address, port_number);
    break;
#endif
  default:
    throw std::invalid_argument("Unsupported address family");
  }
}

socket_address::socket_address(std::string_view host_address,
                               std::string_view port_number) :
  m_family(AddressFamily::IPv4)
{
  std::uint16_t port = parse_port(port_number);

  // Try IPv4 first
  if (host_address.find(':') == std::string_view::npos)
  {
    init_ipv4(host_address, port);
  }
  else
  {
#ifdef AF_INET6
    m_family = AddressFamily::IPv6;
    init_ipv6(host_address, port);
#else
    throw std::invalid_argument("IPv6 not supported on this platform");
#endif
  }
}

socket_address::socket_address(Family family,
                               std::string_view host_address,
                               std::string_view port_number) :
  m_family(family)
{
  std::uint16_t port = parse_port(port_number);

  switch (family)
  {
  case AddressFamily::IPv4:
    init_ipv4(host_address, port);
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    init_ipv6(host_address, port);
    break;
#endif
  default:
    throw std::invalid_argument("Unsupported address family");
  }
}

socket_address::socket_address(std::string_view host_and_port) :
  m_family(AddressFamily::IPv4)
{
  std::string host;
  std::uint16_t port;
  parse_host_and_port(host_and_port, host, port);

  // Try IPv4 first
  if (host.find(':') == std::string::npos)
  {
    init_ipv4(host, port);
  }
  else
  {
#ifdef AF_INET6
    m_family = AddressFamily::IPv6;
    init_ipv6(host, port);
#else
    throw std::invalid_argument("IPv6 not supported on this platform");
#endif
  }
}

socket_address::socket_address(Family family, std::string_view addr) :
  m_family(family)
{
  std::string host;
  std::uint16_t port;
  parse_host_and_port(addr, host, port);

  switch (family)
  {
  case AddressFamily::IPv4:
    init_ipv4(host, port);
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    init_ipv6(host, port);
    break;
#endif
  default:
    throw std::invalid_argument("Unsupported address family");
  }
}

socket_address::socket_address(const socket_address &other) :
  m_family(other.m_family)
{
  std::memcpy(&m_addr, &other.m_addr, sizeof(m_addr));
}

socket_address::socket_address(socket_address &&other) noexcept :
  m_family(other.m_family)
{
  std::memcpy(&m_addr, &other.m_addr, sizeof(m_addr));
}

socket_address::socket_address(const struct sockaddr *addr, socklen_t length)
{
  if (addr->sa_family == AF_INET && length >= sizeof(struct sockaddr_in))
  {
    m_family = AddressFamily::IPv4;
    init_ipv4(reinterpret_cast<const struct sockaddr_in *>(addr));
  }
#ifdef AF_INET6
  else if (addr->sa_family == AF_INET6 && length >= sizeof(struct sockaddr_in6))
  {
    m_family = AddressFamily::IPv6;
    init_ipv6(reinterpret_cast<const struct sockaddr_in6 *>(addr));
  }
#endif
  else
  {
    throw std::invalid_argument("Invalid socket address");
  }
}

socket_address &socket_address::operator=(const socket_address &other)
{
  if (this != &other)
  {
    m_family = other.m_family;
    std::memcpy(&m_addr, &other.m_addr, sizeof(m_addr));
  }
  return *this;
}

socket_address &socket_address::operator=(socket_address &&other) noexcept
{
  if (this != &other)
  {
    m_family = other.m_family;
    std::memcpy(&m_addr, &other.m_addr, sizeof(m_addr));
  }
  return *this;
}

std::string socket_address::host() const
{
  char buffer[INET6_ADDRSTRLEN];

  switch (m_family)
  {
  case AddressFamily::IPv4:
  {
    if (inet_ntop(AF_INET, &m_ipv4_addr.sin_addr, buffer, INET_ADDRSTRLEN))
    {
      return std::string(buffer);
    }
    break;
  }
#ifdef AF_INET6
  case AddressFamily::IPv6:
  {
    if (inet_ntop(AF_INET6, &m_ipv6_addr.sin6_addr, buffer, INET6_ADDRSTRLEN))
    {
      return std::string(buffer);
    }
    break;
  }
#endif
#ifdef AF_UNIX
  case AddressFamily::UNIX_LOCAL:
    // UNIX_LOCAL sockets don't have a host address
    return "localhost";
#endif
  }

  throw std::runtime_error("Failed to convert address to string");
}

std::uint16_t socket_address::port() const
{
  switch (m_family)
  {
  case AddressFamily::IPv4:
    return ntohs(m_ipv4_addr.sin_port);
#ifdef AF_INET6
  case AddressFamily::IPv6:
    return ntohs(m_ipv6_addr.sin6_port);
#endif
  default:
    return 0;
  }
}

socklen_t socket_address::length() const
{
  switch (m_family)
  {
  case AddressFamily::IPv4:
    return sizeof(struct sockaddr_in);
#ifdef AF_INET6
  case AddressFamily::IPv6:
    return sizeof(struct sockaddr_in6);
#endif
  default:
    return sizeof(struct sockaddr);
  }
}

const struct sockaddr *socket_address::addr() const { return &m_addr; }

int socket_address::af() const { return static_cast<int>(m_family); }

std::string socket_address::to_string() const
{
  std::ostringstream oss;

  switch (m_family)
  {
  case AddressFamily::IPv4:
    oss << host() << ":" << port();
    break;
#ifdef AF_INET6
  case AddressFamily::IPv6:
    oss << "[" << host() << "]:" << port();
    break;
#endif
  default:
    oss << "unknown:" << port();
  }

  return oss.str();
}

socket_address::Family socket_address::family() const { return m_family; }

bool socket_address::operator<(const socket_address &other) const
{
  if (m_family != other.m_family)
  {
    return static_cast<int>(m_family) < static_cast<int>(other.m_family);
  }

  const std::string this_host  = host();
  const std::string other_host = other.host();

  if (this_host != other_host)
  {
    return this_host < other_host;
  }

  return port() < other.port();
}

bool socket_address::operator==(const socket_address &other) const
{
  return m_family == other.m_family && host() == other.host() &&
         port() == other.port();
}

bool socket_address::operator!=(const socket_address &other) const
{
  return !(*this == other);
}

void socket_address::init_ipv4()
{
  std::memset(&m_ipv4_addr, 0, sizeof(m_ipv4_addr));
  m_ipv4_addr.sin_family      = AF_INET;
  m_ipv4_addr.sin_addr.s_addr = INADDR_ANY;
  m_ipv4_addr.sin_port        = 0;
}

void socket_address::init_ipv4(std::string_view host, std::uint16_t port)
{
  std::memset(&m_ipv4_addr, 0, sizeof(m_ipv4_addr));
  m_ipv4_addr.sin_family = AF_INET;
  m_ipv4_addr.sin_port   = htons(port);

  if (host == "0.0.0.0" || host.empty())
  {
    m_ipv4_addr.sin_addr.s_addr = INADDR_ANY;
  }
  else
  {
    m_ipv4_addr.sin_addr.s_addr = resolve_host_ipv4(host);
  }
}

void socket_address::init_ipv4(const struct sockaddr_in *addr)
{
  std::memcpy(&m_ipv4_addr, addr, sizeof(m_ipv4_addr));
}

#ifdef AF_INET6
void socket_address::init_ipv6()
{
  std::memset(&m_ipv6_addr, 0, sizeof(m_ipv6_addr));
  m_ipv6_addr.sin6_family = AF_INET6;
  m_ipv6_addr.sin6_addr   = in6addr_any;
  m_ipv6_addr.sin6_port   = 0;
}

void socket_address::init_ipv6(std::string_view host, std::uint16_t port)
{
  std::memset(&m_ipv6_addr, 0, sizeof(m_ipv6_addr));
  m_ipv6_addr.sin6_family = AF_INET6;
  m_ipv6_addr.sin6_port   = htons(port);

  if (host == "::" || host.empty())
  {
    m_ipv6_addr.sin6_addr = in6addr_any;
  }
  else
  {
    resolve_host_ipv6(host, m_ipv6_addr);
  }
}

void socket_address::init_ipv6(const struct sockaddr_in6 *addr)
{
  std::memcpy(&m_ipv6_addr, addr, sizeof(m_ipv6_addr));
}
#endif

void socket_address::parse_host_and_port(std::string_view host_and_port,
                                         std::string &host,
                                         std::uint16_t &port)
{
  // Handle IPv6 addresses in brackets [host]:port
  if (host_and_port.front() == '[')
  {
    auto bracket_pos = host_and_port.find(']');
    if (bracket_pos == std::string_view::npos)
    {
      throw std::invalid_argument("Invalid IPv6 address format");
    }

    host           = std::string(host_and_port.substr(1, bracket_pos - 1));

    auto colon_pos = host_and_port.find(':', bracket_pos);
    if (colon_pos == std::string_view::npos)
    {
      throw std::invalid_argument("Missing port number");
    }

    port = parse_port(host_and_port.substr(colon_pos + 1));
  }
  else
  {
    // IPv4 format host:port
    auto colon_pos = host_and_port.rfind(':');
    if (colon_pos == std::string_view::npos)
    {
      throw std::invalid_argument("Missing port number");
    }

    host = std::string(host_and_port.substr(0, colon_pos));
    port = parse_port(host_and_port.substr(colon_pos + 1));
  }
}

std::uint16_t socket_address::parse_port(std::string_view port_str)
{
  if (port_str.empty())
  {
    throw std::invalid_argument("Empty port string");
  }

  try
  {
    int port_val = std::stoi(std::string(port_str));
    if (port_val < 0 || port_val > 65535)
    {
      throw std::invalid_argument("Port number out of range");
    }
    return static_cast<std::uint16_t>(port_val);
  }
  catch (const std::invalid_argument &)
  {
    // Try to resolve service name
    struct servent *service =
        getservbyname(std::string(port_str).c_str(), nullptr);
    if (service)
    {
      return ntohs(service->s_port);
    }
    throw std::invalid_argument("Invalid port number or service name");
  }
}

std::uint32_t socket_address::resolve_host_ipv4(std::string_view host)
{
  std::uint32_t addr;

  // Try to parse as dotted decimal first
  if (inet_pton(AF_INET, std::string(host).c_str(), &addr) == 1)
  {
    return addr;
  }

  // Try DNS resolution
  struct addrinfo hints   = {};
  hints.ai_family         = AF_INET;
  hints.ai_socktype       = SOCK_STREAM;

  struct addrinfo *result = nullptr;
  int error = getaddrinfo(std::string(host).c_str(), nullptr, &hints, &result);

  if (error != 0)
  {
    throw std::runtime_error(format_gai_error(host, error));
  }

  addr =
      reinterpret_cast<struct sockaddr_in *>(result->ai_addr)->sin_addr.s_addr;
  freeaddrinfo(result);

  return addr;
}

#ifdef AF_INET6
void socket_address::resolve_host_ipv6(std::string_view host,
                                       struct sockaddr_in6 &addr)
{
  // Try to parse as IPv6 address first
  if (inet_pton(AF_INET6, std::string(host).c_str(), &addr.sin6_addr) == 1)
  {
    return;
  }

  // Try DNS resolution
  struct addrinfo hints   = {};
  hints.ai_family         = AF_INET6;
  hints.ai_socktype       = SOCK_STREAM;

  struct addrinfo *result = nullptr;
  int error = getaddrinfo(std::string(host).c_str(), nullptr, &hints, &result);

  if (error != 0)
  {
    throw std::runtime_error(format_gai_error(host, error));
  }

  std::memcpy(
      &addr.sin6_addr,
      &reinterpret_cast<struct sockaddr_in6 *>(result->ai_addr)->sin6_addr,
      sizeof(addr.sin6_addr));
  freeaddrinfo(result);
}
#endif

std::ostream &operator<<(std::ostream &ostr, const socket_address &address)
{
  ostr << address.to_string();
  return ostr;
}

} // namespace fb
