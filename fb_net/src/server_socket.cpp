#include <fb/server_socket.h>
#include <algorithm>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

namespace fb {

/**
 * @brief Default constructor creates an uninitialized TCP server socket
 */
server_socket::server_socket() :
  socket_base(),
  m_backlog(DEFAULT_BACKLOG)
{
  init_server_socket(socket_address::IPv4);
}

/**
 * @brief Create a TCP server socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
server_socket::server_socket(socket_address::Family family) :
  socket_base(),
  m_backlog(DEFAULT_BACKLOG)
{
  init_server_socket(family);
}

/**
 * @brief Create a TCP server socket and bind to port (any address)
 * @param port Port number to bind to
 * @param backlog Listen backlog size
 * @throws std::system_error, std::invalid_argument
 */
server_socket::server_socket(std::uint16_t port, int backlog) :
  socket_base(),
  m_backlog(std::min(backlog, MAX_BACKLOG))
{
  init_server_socket(socket_address::IPv4);
  socket_address address(port);
  setup_server(address, m_backlog, true);
}

/**
 * @brief Create a TCP server socket and bind to specific address
 * @param address Local address to bind to
 * @param backlog Listen backlog size
 * @throws std::system_error, std::invalid_argument
 */
server_socket::server_socket(const socket_address &address, int backlog) :
  socket_base(),
  m_backlog(std::min(backlog, MAX_BACKLOG))
{
  init_server_socket(address.family());
  setup_server(address, m_backlog, true);
}

/**
 * @brief Create a TCP server socket, bind and start listening
 * @param address Local address to bind to
 * @param backlog Listen backlog size
 * @param reuse_address Enable SO_REUSEADDR option
 * @throws std::system_error, std::invalid_argument
 */
server_socket::server_socket(const socket_address &address,
                             int backlog,
                             bool reuse_address) :
  socket_base(),
  m_backlog(std::min(backlog, MAX_BACKLOG))
{
  init_server_socket(address.family());
  setup_server(address, m_backlog, reuse_address);
}

/**
 * @brief Move constructor
 */
server_socket::server_socket(server_socket &&other) noexcept :
  socket_base(std::move(other)),
  m_backlog(other.m_backlog)
{
  other.m_backlog = DEFAULT_BACKLOG;
}

server_socket &server_socket::operator=(server_socket &&other) noexcept
{
  if (this != &other)
  {
    socket_base::operator=(std::move(other));
    m_backlog       = other.m_backlog;
    other.m_backlog = DEFAULT_BACKLOG;
  }
  return *this;
}

/**
 * @brief Bind server socket to local address
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR option
 * @throws std::system_error, std::invalid_argument
 */
void server_socket::bind(const socket_address &address, bool reuse_address)
{
  try
  {
    socket_base::bind(address, reuse_address);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Bind server socket with port reuse option
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR option
 * @param reuse_port Enable SO_REUSEPORT option
 * @throws std::system_error, std::invalid_argument
 */
void server_socket::bind(const socket_address &address,
                         bool reuse_address,
                         bool reuse_port)
{
  try
  {
    socket_base::bind(address, reuse_address, reuse_port);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Start listening for incoming connections
 * @param backlog Maximum number of pending connections (default 64)
 * @throws std::system_error
 */
void server_socket::listen(int backlog)
{
  if (backlog > 0)
  {
    m_backlog = std::min(backlog, MAX_BACKLOG);
  }

  try
  {
    socket_base::listen(m_backlog);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Accept an incoming connection
 * @param client_address Output parameter for client address
 * @return tcp_client instance for the accepted connection
 * @throws std::system_error
 */
tcp_client server_socket::accept_connection(socket_address &client_address)
{
  check_initialized();

  // Accept connection at the socket level
  sockaddr_storage client_addr_storage;
  socklen_t client_addr_len = sizeof(client_addr_storage);

  socket_t client_fd =
      ::accept(sockfd(), reinterpret_cast<sockaddr *>(&client_addr_storage),
               &client_addr_len);

  if (client_fd == INVALID_SOCKET_VALUE)
  {
    error("Failed to accept connection");
  }

  // Convert sockaddr to socket_address
  client_address = socket_address(
      reinterpret_cast<sockaddr *>(&client_addr_storage), client_addr_len);

  return tcp_client(client_fd);
}

/**
 * @brief Accept an incoming connection
 * @return tcp_client instance for the accepted connection
 * @throws std::system_error
 */
tcp_client server_socket::accept_connection()
{
  socket_address client_address;
  return accept_connection(client_address);
}

/**
 * @brief Accept an incoming connection with timeout
 * @param client_address Output parameter for client address
 * @param timeout Accept timeout
 * @return tcp_client instance for the accepted connection
 * @throws std::system_error
 */
tcp_client
server_socket::accept_connection(socket_address &client_address,
                                 const std::chrono::milliseconds &timeout)
{
  check_initialized();

  // Check if connection is available within timeout
  if (!poll(timeout, socket_base::SELECT_READ))
  {
    throw std::system_error(std::make_error_code(std::errc::timed_out),
                            "Accept operation timed out");
  }

  // Now accept should not block since we know a connection is available
  return accept_connection(client_address);
}

/**
 * @brief Accept an incoming connection with timeout
 * @param timeout Accept timeout
 * @return tcp_client instance for the accepted connection
 * @throws std::system_error
 */
tcp_client
server_socket::accept_connection(const std::chrono::milliseconds &timeout)
{
  socket_address client_address;
  return accept_connection(client_address, timeout);
}

/**
 * @brief Set the listen backlog size
 * @param backlog Maximum number of pending connections
 * @throws std::system_error
 */
void server_socket::set_backlog(int backlog)
{
  if (backlog < 0)
  {
    throw std::invalid_argument("Backlog cannot be negative");
  }

  m_backlog = std::min(backlog, MAX_BACKLOG);

  // If socket is already listening, we need to call listen again
  // Note: This may not be portable across all platforms
  if (!is_closed())
  {
    try
    {
      listen(m_backlog);
    }
    catch (const std::exception &)
    {
      // Some platforms don't allow changing backlog after listen
      // In this case, we just update our internal value
    }
  }
}

/**
 * @brief Enable/disable address reuse (SO_REUSEADDR)
 * @param flag Enable/disable address reuse
 * @throws std::system_error
 */
void server_socket::set_reuse_address(bool flag)
{
  socket_base::set_reuse_address(flag);
}

/**
 * @brief Get address reuse setting
 * @return True if address reuse is enabled
 * @throws std::system_error
 */
bool server_socket::get_reuse_address()
{
  return socket_base::get_reuse_address();
}

/**
 * @brief Enable/disable port reuse (SO_REUSEPORT)
 * @param flag Enable/disable port reuse
 * @throws std::system_error
 */
void server_socket::set_reuse_port(bool flag)
{
  socket_base::set_reuse_port(flag);
}

/**
 * @brief Get port reuse setting
 * @return True if port reuse is enabled
 * @throws std::system_error
 */
bool server_socket::get_reuse_port() { return socket_base::get_reuse_port(); }

/**
 * @brief Check if there are pending connections
 * @param timeout Poll timeout
 * @return True if connections are pending
 * @throws std::system_error
 */
bool server_socket::has_pending_connections(
    const std::chrono::milliseconds &timeout)
{
  if (is_closed())
  {
    return false;
  }

  try
  {
    return poll(timeout, socket_base::SELECT_READ);
  }
  catch (const std::exception &)
  {
    return false;
  }
}

/**
 * @brief Get the maximum number of connections that can be queued
 * @return Maximum queue size
 */
int server_socket::max_connections() const { return MAX_BACKLOG; }

/**
 * @brief Get current socket queue size (if supported by platform)
 * @return Number of pending connections, or -1 if not supported
 */
int server_socket::queue_size() const
{
  // Most platforms don't provide a direct way to query current queue size
  // This would require platform-specific implementation
  // For now, return -1 to indicate not supported
  return -1;
}

/**
 * @brief Initialize TCP server socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
void server_socket::init_server_socket(socket_address::Family family)
{
  try
  {
    init(family, STREAM_SOCKET);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Setup server socket with bind and listen operations
 * @param address Address to bind to
 * @param backlog Listen backlog
 * @param reuse_address Enable address reuse
 * @param reuse_port Enable port reuse
 */
void server_socket::setup_server(const socket_address &address,
                                 int backlog,
                                 bool reuse_address,
                                 bool reuse_port)
{
  try
  {
    // Set socket options before binding
    if (reuse_address)
    {
      set_reuse_address(true);
    }
    if (reuse_port)
    {
      set_reuse_port(true);
    }

    // Bind to the address
    bind(address, reuse_address, reuse_port);

    // Start listening
    listen(backlog);
  }
  catch (const std::exception &)
  {
    // Clean up on failure
    try
    {
      close();
    }
    catch (...)
    {
      // Ignore cleanup errors
    }
    throw;
  }
}

} // namespace fb
