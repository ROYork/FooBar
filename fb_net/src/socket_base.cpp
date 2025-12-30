#include <fb/socket_base.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <string>
#include <utility>

#ifdef _WIN32
#include <mswsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

// Ensure SO_EXCLUSIVEADDRUSE is defined (should be in winsock2.h for Windows 2000+)
#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))
#endif
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace
{

#ifdef _WIN32
std::error_code make_error_code_from_native(int native_code)
{
  switch (native_code)
  {
  case WSAECONNREFUSED:
    return std::make_error_code(std::errc::connection_refused);
  case WSAETIMEDOUT:
    return std::make_error_code(std::errc::timed_out);
  case WSAEWOULDBLOCK:
    return std::make_error_code(std::errc::resource_unavailable_try_again);
  case WSAEINPROGRESS:
    return std::make_error_code(std::errc::operation_in_progress);
  case WSAEHOSTUNREACH:
  case WSAENETUNREACH:
    return std::make_error_code(std::errc::host_unreachable);
  case WSAECONNRESET:
    return std::make_error_code(std::errc::connection_reset);
  case WSAECONNABORTED:
    return std::make_error_code(std::errc::connection_aborted);
  default:
    // For unmapped codes, use system_category which preserves the native code
    // Note: On Windows, non-Winsock error codes (like errno values) should not occur
    // If they do, it indicates a platform compatibility issue
    return std::error_code(native_code, std::system_category());
  }
}

std::string describe_native_error(int native_code)
{
  if (native_code == 0)
  {
    return {};
  }

  LPSTR buffer = nullptr;
  DWORD flags  = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS;
  DWORD length = FormatMessageA(flags, nullptr, static_cast<DWORD>(native_code),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

  std::string result;
  if (length != 0 && buffer != nullptr)
  {
    result.assign(buffer, length);
    while (!result.empty() &&
           (result.back() == '\r' || result.back() == '\n'))
    {
      result.pop_back();
    }
    LocalFree(buffer);
  }

  return result;
}
#else
std::error_code make_error_code_from_native(int native_code)
{
  switch (native_code)
  {
  case ECONNREFUSED:
    return std::make_error_code(std::errc::connection_refused);
  case ETIMEDOUT:
    return std::make_error_code(std::errc::timed_out);
  case EAGAIN:
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
  case EWOULDBLOCK:
#endif
    return std::make_error_code(std::errc::resource_unavailable_try_again);
  case EINPROGRESS:
    return std::make_error_code(std::errc::operation_in_progress);
  case EHOSTUNREACH:
    return std::make_error_code(std::errc::host_unreachable);
  default:
    return std::error_code(native_code, std::generic_category());
  }
}

std::string describe_native_error(int native_code)
{
  if (native_code == 0)
  {
    return {};
  }

  return std::string(strerror(native_code));
}
#endif

std::string compose_error_message(const std::string &context, int native_code)
{
  const std::string description = describe_native_error(native_code);
  std::string message;

  if (!context.empty())
  {
    message += context;
    if (!description.empty())
    {
      message += ": ";
    }
  }

  if (!description.empty())
  {
    message += description;
  }

  if (native_code != 0)
  {
    if (!message.empty())
    {
      message += " ";
    }
    message += "(code: " + std::to_string(native_code) + ")";
  }

  if (message.empty())
  {
    message = "Socket error (code: " + std::to_string(native_code) + ")";
  }

  return message;
}

} // namespace

namespace fb
{

// Static member initialization
#ifdef _WIN32
bool socket_base::s_winsock_initialized = false;
int socket_base::s_winsock_ref_count    = 0;
#endif

/**
 * @brief Default constructor creates an uninitialized socket
 */
socket_base::socket_base() :
  m_sockfd(INVALID_SOCKET_VALUE),
  m_is_connected(false),
  m_is_closed(true),
  m_recv_timeout(std::chrono::milliseconds(0)),
  m_send_timeout(std::chrono::milliseconds(0)),
  m_blocking(true),
  m_is_broken_timeout(false)
{
#ifdef _WIN32
  init_winsock();
#endif
}

/**
 * @brief Create a socket with specific address family and type
 * @param family Address family (IPv4, IPv6)
 * @param type Socket type (STREAM, DATAGRAM, RAW)
 */
socket_base::socket_base(socket_address::Family family, Type type) :
  m_sockfd(INVALID_SOCKET_VALUE),
  m_is_connected(false),
  m_is_closed(true),
  m_recv_timeout(std::chrono::milliseconds(0)),
  m_send_timeout(std::chrono::milliseconds(0)),
  m_blocking(true),
  m_is_broken_timeout(false)
{
#ifdef _WIN32
  init_winsock();
#endif
  init(family, type);
}

/**
 * @brief Create socket from existing socket descriptor (for accept operations)
 * @param sockfd Socket descriptor to wrap
 */
socket_base::socket_base(socket_t sockfd) :
  m_sockfd(sockfd),
  m_is_connected(true),
  m_is_closed(false),
  m_recv_timeout(std::chrono::milliseconds(0)),
  m_send_timeout(std::chrono::milliseconds(0)),
  m_blocking(true),
  m_is_broken_timeout(false)
{
#ifdef _WIN32
  init_winsock();
#endif
}

/**
 * @brief Move constructor
 */
socket_base::socket_base(socket_base &&other) noexcept :
  m_sockfd(other.m_sockfd),
  m_is_connected(other.m_is_connected),
  m_is_closed(other.m_is_closed),
  m_recv_timeout(other.m_recv_timeout),
  m_send_timeout(other.m_send_timeout),
  m_blocking(other.m_blocking),
  m_is_broken_timeout(other.m_is_broken_timeout)
{
  other.m_sockfd       = INVALID_SOCKET_VALUE;
  other.m_is_connected = false;
  other.m_is_closed    = true;
}

/**
 * @brief Move assignment operator
 */
socket_base &socket_base::operator=(socket_base &&other) noexcept
{
  if (this != &other)
  {
    // Close current socket if open
    if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed)
    {
      try
      {
        close();
      }
      catch (...)
      {
        // Ignore exceptions during move assignment cleanup
      }
    }

    // Move from other
    m_sockfd            = other.m_sockfd;
    m_is_connected      = other.m_is_connected;
    m_is_closed         = other.m_is_closed;
    m_recv_timeout      = other.m_recv_timeout;
    m_send_timeout      = other.m_send_timeout;
    m_blocking          = other.m_blocking;
    m_is_broken_timeout = other.m_is_broken_timeout;

    // Reset other
    other.m_sockfd       = INVALID_SOCKET_VALUE;
    other.m_is_connected = false;
    other.m_is_closed    = true;
  }
  return *this;
}

/**
 * @brief Destructor - automatically closes socket if open
 */
socket_base::~socket_base()
{
  if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed)
  {
    try
    {
      close();
    }
    catch (...)
    {
      // Ignore exceptions in destructor
    }
  }

#ifdef _WIN32
  cleanup_winsock();
#endif
}

/**
 * @brief Close the socket
 * @throws std::system_error on error
 */
void socket_base::close()
{
  if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed)
  {
    try
    {
#ifdef _WIN32
      closesocket(m_sockfd);
#else
      ::close(m_sockfd);
#endif
      m_sockfd       = INVALID_SOCKET_VALUE;
      m_is_connected = false;
      m_is_closed    = true;
    }
    catch (const std::exception &)
    {
      m_is_connected = false;
      m_is_closed    = true;
      throw;
    }
  }
}

/**
 * @brief Bind socket to local address
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR
 * @throws std::system_error, std::invalid_argument
 */
void socket_base::bind(const socket_address &address, bool reuse_address)
{
  bind(address, reuse_address, false);
}

/**
 * @brief Bind socket to local address with port reuse option
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR
 * @param reuse_port Enable SO_REUSEPORT
 * @throws std::system_error, std::invalid_argument
 */
void socket_base::bind(const socket_address &address,
                       bool reuse_address,
                       bool reuse_port)
{
  check_initialized();
  try
  {
    if (m_sockfd == INVALID_SOCKET_VALUE)
    {
      init(address.af());
    }

    if (reuse_address)
    {
      set_reuse_address(true);
    }

    if (reuse_port)
    {
      set_reuse_port(true);
    }

    int result = ::bind(m_sockfd, address.addr(), address.length());

    if (result != 0)
    {
      error("bind");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Start listening for connections (server sockets)
 * @param backlog Maximum number of pending connections
 * @throws std::system_error
 */
void socket_base::listen(int backlog)
{
  check_initialized();
  try
  {
    int result = ::listen(m_sockfd, backlog);

    if (result != 0)
    {
      error("listen");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Connect to remote address (client sockets)
 * @param address Remote address to connect to
 * @throws std::system_error
 */
void socket_base::connect(const socket_address &address)
{
  check_initialized();
  try
  {
    if (m_sockfd == INVALID_SOCKET_VALUE)
    {
      init(address.af());
    }

    int result = ::connect(m_sockfd, address.addr(), address.length());

    if (result != 0)
    {
      error("connect");
    }

    m_is_connected = true;
    m_is_closed    = false;
  }
  catch (const std::exception &)
  {
    m_is_connected = false;
    throw;
  }
}

/**
 * @brief Connect to remote address with timeout
 * @param address Remote address to connect to
 * @param timeout Connection timeout
 * @throws std::system_error
 */
void socket_base::connect(const socket_address &address,
                          const std::chrono::milliseconds &timeout)
{
  check_initialized();
  try
  {
    if (m_sockfd == INVALID_SOCKET_VALUE)
    {
      init(address.af());
    }

    set_blocking(false);

    int result = ::connect(m_sockfd, address.addr(), address.length());

    if (result == 0)
    {
      set_blocking(true);
      m_is_connected = true;
      m_is_closed    = false;
      return;
    }

#ifdef _WIN32
    int err = WSAGetLastError();
    if (err != WSAEWOULDBLOCK)
    {
      set_blocking(true);
      error("connect");
    }
#else
    if (errno != EINPROGRESS)
    {
      set_blocking(true);
      error("connect");
    }
#endif

    if (!poll(timeout, SELECT_WRITE))
    {
      set_blocking(true);
      throw std::system_error(std::make_error_code(std::errc::timed_out),
                              "Connection timeout");
    }

    // Check for connection errors
    // Note: Windows SO_ERROR behavior differs from Unix - it may not always
    // contain the correct error code. We handle this by checking both SO_ERROR
    // and attempting a second connect() call on Windows.
    int opt_val = 0;
    socklen_t opt_len = sizeof(opt_val);
    if (getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR,
                   reinterpret_cast<char *>(&opt_val), &opt_len) < 0)
    {
      set_blocking(true);
      error("getsockopt");
    }

#ifdef _WIN32
    // On Windows, SO_ERROR is unreliable for non-blocking connects
    // It may return 0, errno values, or incorrect codes
    // Always use a second connect() call to get the correct Winsock error code
    int connect_result = ::connect(m_sockfd, address.addr(), address.length());
    if (connect_result != 0)
    {
      int connect_err = WSAGetLastError();
      // Note: For non-blocking connect() on Windows, an additional connect() call
      // provides accurate error status; WSAEISCONN indicates success.

      // WSAEISCONN (10056) means connection already established - this is success
      if (connect_err != WSAEISCONN)
      {
        set_blocking(true);
        error(connect_err, "connect");
      }
    }
    // If connect_result == 0 or error == WSAEISCONN, connection succeeded
#else
    // Unix/Linux: SO_ERROR is reliable
    if (opt_val != 0)
    {
      set_blocking(true);
      error(opt_val, "connect");
    }
#endif

    set_blocking(true);
    m_is_connected = true;
    m_is_closed    = false;
  }
  catch (const std::exception &)
  {
    m_is_connected = false;
    throw;
  }
}

/**
 * @brief Accept incoming connection (server sockets)
 * @param client_address Output parameter for client address
 * @return New Socket instance for the connection
 * @throws std::system_error
 */
socket_base socket_base::accept_connection(socket_address &client_address)
{
  check_initialized();
  try
  {
    struct sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);

    socket_t client_fd =
        ::accept(m_sockfd, reinterpret_cast<struct sockaddr *>(&addr_storage),
                 &addr_len);

    if (client_fd == INVALID_SOCKET_VALUE)
    {
      error("accept");
    }

    client_address = socket_address(
        reinterpret_cast<struct sockaddr *>(&addr_storage), addr_len);

    return socket_base(client_fd);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Accept incoming connection (server sockets)
 * @return New Socket instance for the connection
 * @throws std::system_error
 */
socket_base socket_base::accept_connection()
{
  socket_address client_address;
  return accept_connection(client_address);
}

/**
 * @brief Set socket option
 * @param level Protocol level (SOL_SOCKET, IPPROTO_TCP, etc.)
 * @param option Option name
 * @param value Option value
 * @throws std::system_error
 */
void socket_base::set_option(int level, int option, int value)
{
  check_initialized();
  try
  {
    if (setsockopt(m_sockfd, level, option,
                   reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
    {
      error("setsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set socket option
 * @param level Protocol level
 * @param option Option name
 * @param value Option value
 * @throws std::system_error
 */
void socket_base::set_option(int level, int option, unsigned int value)
{
  check_initialized();
  try
  {
    if (setsockopt(m_sockfd, level, option,
                   reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
    {
      error("setsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set socket option
 * @param level Protocol level
 * @param option Option name
 * @param value Option value
 * @throws std::system_error
 */
void socket_base::set_option(int level, int option, unsigned char value)
{
  check_initialized();
  try
  {
    if (setsockopt(m_sockfd, level, option,
                   reinterpret_cast<const char *>(&value), sizeof(value)) != 0)
    {
      error("setsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get socket option
 * @param level Protocol level
 * @param option Option name
 * @param value Output parameter for option value
 * @throws std::system_error
 */
void socket_base::get_option(int level, int option, int &value)
{
  check_initialized();
  try
  {
    socklen_t length = sizeof(value);
    if (getsockopt(m_sockfd, level, option, reinterpret_cast<char *>(&value),
                   &length) != 0)
    {
      error("getsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get socket option
 * @param level Protocol level
 * @param option Option name
 * @param value Output parameter for option value
 * @throws std::system_error
 */
void socket_base::get_option(int level, int option, unsigned int &value)
{
  check_initialized();
  try
  {
    socklen_t length = sizeof(value);
    if (getsockopt(m_sockfd, level, option, reinterpret_cast<char *>(&value),
                   &length) != 0)
    {
      error("getsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get socket option
 * @param level Protocol level
 * @param option Option name
 * @param value Output parameter for option value
 * @throws std::system_error
 */
void socket_base::get_option(int level, int option, unsigned char &value)
{
  check_initialized();
  try
  {
    socklen_t length = sizeof(value);
    if (getsockopt(m_sockfd, level, option, reinterpret_cast<char *>(&value),
                   &length) != 0)
    {
      error("getsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set linger option
 * @param on Enable/disable linger
 * @param seconds Linger time in seconds
 * @throws std::system_error
 */
void socket_base::set_linger(bool on, int seconds)
{
  check_initialized();
  try
  {
    struct linger l;
    l.l_onoff  = on ? 1 : 0;
    l.l_linger = static_cast<u_short>(seconds);
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_LINGER,
                   reinterpret_cast<const char *>(&l), sizeof(l)) != 0)
    {
      error("setsockopt");
    }
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get linger option
 * @param on Output parameter for linger enable/disable
 * @param seconds Output parameter for linger time
 * @throws std::system_error
 */
void socket_base::get_linger(bool &on, int &seconds)
{
  check_initialized();
  try
  {
    struct linger l;
    socklen_t length = sizeof(l);
    if (getsockopt(m_sockfd, SOL_SOCKET, SO_LINGER,
                   reinterpret_cast<char *>(&l), &length) != 0)
    {
      error("getsockopt");
    }
    on      = l.l_onoff != 0;
    seconds = l.l_linger;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set TCP no delay option (disable Nagle algorithm)
 * @param flag Enable/disable no delay
 * @throws std::system_error
 */
void socket_base::set_no_delay(bool flag)
{
  check_initialized();
  try
  {
    int value = flag ? 1 : 0;
    set_option(IPPROTO_TCP, TCP_NODELAY, value);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get TCP no delay option
 * @return True if no delay is enabled
 * @throws std::system_error
 */
bool socket_base::get_no_delay()
{
  check_initialized();
  try
  {
    int value;
    get_option(IPPROTO_TCP, TCP_NODELAY, value);
    return value != 0;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set keep alive option
 * @param flag Enable/disable keep alive
 * @throws std::system_error
 */
void socket_base::set_keep_alive(bool flag)
{
  check_initialized();
  try
  {
    int value = flag ? 1 : 0;
    set_option(SOL_SOCKET, SO_KEEPALIVE, value);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get keep alive option
 * @return True if keep alive is enabled
 * @throws std::system_error
 */
bool socket_base::get_keep_alive()
{
  check_initialized();
  try
  {
    int value;
    get_option(SOL_SOCKET, SO_KEEPALIVE, value);
    return value != 0;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set address reuse option
 * @param flag Enable/disable address reuse
 * @throws std::system_error
 */
void socket_base::set_reuse_address(bool flag)
{
  check_initialized();
  try
  {
#ifdef _WIN32
    // Windows SO_REUSEADDR behavior differs significantly from Unix:
    // - Unix: Allows binding to TIME_WAIT sockets, but NOT to actively listening ports
    // - Windows: Allows complete duplicate binding (port hijacking risk)
    //
    // To get Unix-like behavior on Windows:
    // - When reuse=false: Use SO_EXCLUSIVEADDRUSE to prevent any address reuse
    // - When reuse=true: Use SO_REUSEADDR (but be aware of security implications)
    if (!flag)
    {
      // Exclusive mode - prevent any reuse (stronger than Unix behavior)
      int value = 1;
      set_option(SOL_SOCKET, SO_EXCLUSIVEADDRUSE, value);
    }
    else
    {
      // Reuse mode - use standard SO_REUSEADDR
      // Note: This is more permissive than Unix and may allow duplicate bindings
      int value = 1;
      set_option(SOL_SOCKET, SO_REUSEADDR, value);
    }
#else
    // Unix/Linux/macOS: Standard SO_REUSEADDR behavior
    int value = flag ? 1 : 0;
    set_option(SOL_SOCKET, SO_REUSEADDR, value);
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get address reuse option
 * @return True if address reuse is enabled
 * @throws std::system_error
 */
bool socket_base::get_reuse_address()
{
  check_initialized();
  try
  {
    int value;
    get_option(SOL_SOCKET, SO_REUSEADDR, value);
    return value != 0;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set port reuse option
 * @param flag Enable/disable port reuse
 * @throws std::system_error
 */
void socket_base::set_reuse_port(bool flag)
{
  check_initialized();
  try
  {
#ifdef SO_REUSEPORT
    int value = flag ? 1 : 0;
    set_option(SOL_SOCKET, SO_REUSEPORT, value);
#else
    // SO_REUSEPORT not supported on this platform
    (void)flag;  // Suppress unused parameter warning
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get port reuse option
 * @return True if port reuse is enabled
 * @throws std::system_error
 */
bool socket_base::get_reuse_port()
{
  check_initialized();
  try
  {
#ifdef SO_REUSEPORT
    int value;
    get_option(SOL_SOCKET, SO_REUSEPORT, value);
    return value != 0;
#else
    return false;
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set broadcast option
 * @param flag Enable/disable broadcast
 * @throws std::system_error
 */
void socket_base::set_broadcast(bool flag)
{
  check_initialized();
  try
  {
    int value = flag ? 1 : 0;
    set_option(SOL_SOCKET, SO_BROADCAST, value);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get broadcast option
 * @return True if broadcast is enabled
 * @throws std::system_error
 */
bool socket_base::get_broadcast()
{
  check_initialized();
  try
  {
    int value;
    get_option(SOL_SOCKET, SO_BROADCAST, value);
    return value != 0;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set blocking mode
 * @param flag True for blocking, false for non-blocking
 * @throws std::system_error
 */
void socket_base::set_blocking(bool flag)
{
  check_initialized();
  try
  {
#ifdef _WIN32
    u_long arg = flag ? 0 : 1;
    if (ioctlsocket(m_sockfd, FIONBIO, &arg) != 0)
    {
      error("ioctlsocket");
    }
#else
    int flags = fcntl(m_sockfd, F_GETFL, 0);
    if (flags == -1)
    {
      error("fcntl");
    }

    if (flag)
    {
      flags &= ~O_NONBLOCK;
    }
    else
    {
      flags |= O_NONBLOCK;
    }

    if (fcntl(m_sockfd, F_SETFL, flags) == -1)
    {
      error("fcntl");
    }
#endif

    m_blocking = flag;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set send buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void socket_base::set_send_buffer_size(int size)
{
  check_initialized();
  try
  {
    set_option(SOL_SOCKET, SO_SNDBUF, size);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get send buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int socket_base::get_send_buffer_size()
{
  check_initialized();
  try
  {
    int result;
    get_option(SOL_SOCKET, SO_SNDBUF, result);
    return result;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set receive buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void socket_base::set_receive_buffer_size(int size)
{
  check_initialized();
  try
  {
    set_option(SOL_SOCKET, SO_RCVBUF, size);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get receive buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int socket_base::get_receive_buffer_size()
{
  check_initialized();
  try
  {
    int result;
    get_option(SOL_SOCKET, SO_RCVBUF, result);
    return result;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set send timeout
 * @param timeout Send timeout
 * @throws std::system_error
 */
void socket_base::set_send_timeout(const std::chrono::milliseconds &timeout)
{
  check_initialized();
  try
  {
    m_send_timeout = timeout;

#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout.count());
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char *>(&tv), sizeof(tv)) != 0)
    {
      error("setsockopt");
    }
#else
    struct timeval tv;
    tv.tv_sec  = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0)
    {
      error("setsockopt");
    }
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get send timeout
 * @return Send timeout
 * @throws std::system_error
 */
std::chrono::milliseconds socket_base::get_send_timeout()
{
  check_initialized();
  try
  {
    return m_send_timeout;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Set receive timeout
 * @param timeout Receive timeout
 * @throws std::system_error
 */
void socket_base::set_receive_timeout(const std::chrono::milliseconds &timeout)
{
  check_initialized();
  try
  {
    m_recv_timeout = timeout;

#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout.count());
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char *>(&tv), sizeof(tv)) != 0)
    {
      error("setsockopt");
    }
#else
    struct timeval tv;
    tv.tv_sec  = timeout.count() / 1000;
    tv.tv_usec = (timeout.count() % 1000) * 1000;
    if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
    {
      error("setsockopt");
    }
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get receive timeout
 * @return Receive timeout
 * @throws std::system_error
 */
std::chrono::milliseconds socket_base::get_receive_timeout()
{
  check_initialized();
  try
  {
    return m_recv_timeout;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get local socket address
 * @return Local socket address
 * @throws std::system_error
 */
socket_address socket_base::address() const
{
  check_initialized();
  try
  {
    struct sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);

    if (getsockname(m_sockfd,
                    reinterpret_cast<struct sockaddr *>(&addr_storage),
                    &addr_len) != 0)
    {
#ifdef _WIN32
      // On Windows, getsockname() fails with WSAEINVAL (10022) if socket is not yet bound
      // Return a default wildcard address to match Unix behavior (0.0.0.0:0)
      int err = last_error();
      if (err == WSAEINVAL)
      {
        // Try to get the socket family to return the correct address family
        // Use getsockopt with SO_PROTOCOL_INFO (Windows-specific) or fall back to IPv4
        WSAPROTOCOL_INFOW protocol_info;
        int opt_len = sizeof(protocol_info);
        if (getsockopt(m_sockfd, SOL_SOCKET, SO_PROTOCOL_INFO,
                      reinterpret_cast<char*>(&protocol_info), &opt_len) == 0)
        {
          return socket_address(static_cast<socket_address::Family>(protocol_info.iAddressFamily));
        }
        // Fall back to IPv4 if we can't determine the family
        return socket_address(socket_address::Family::IPv4);
      }
#endif
      error("getsockname");
    }

    return socket_address(reinterpret_cast<struct sockaddr *>(&addr_storage),
                          addr_len);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get peer socket address (for connected sockets)
 * @return Peer socket address
 * @throws std::system_error
 */
socket_address socket_base::peer_address() const
{
  check_initialized();
  try
  {
    struct sockaddr_storage addr_storage;
    socklen_t addr_len = sizeof(addr_storage);

    if (getpeername(m_sockfd,
                    reinterpret_cast<struct sockaddr *>(&addr_storage),
                    &addr_len) != 0)
    {
#ifdef _WIN32
      // On Windows, getpeername() fails with WSAENOTCONN (10057) if socket is not connected
      // Return a default wildcard address to match Unix behavior
      int err = last_error();
      if (err == WSAENOTCONN || err == WSAEINVAL)
      {
        // Try to get the socket family to return the correct address family
        WSAPROTOCOL_INFOW protocol_info;
        int opt_len = sizeof(protocol_info);
        if (getsockopt(m_sockfd, SOL_SOCKET, SO_PROTOCOL_INFO,
                      reinterpret_cast<char*>(&protocol_info), &opt_len) == 0)
        {
          return socket_address(static_cast<socket_address::Family>(protocol_info.iAddressFamily));
        }
        // Fall back to IPv4 if we can't determine the family
        return socket_address(socket_address::Family::IPv4);
      }
#endif
      error("getpeername");
    }

    return socket_address(reinterpret_cast<struct sockaddr *>(&addr_storage),
                          addr_len);
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Check if socket is connected
 * @return True if socket is connected
 */
bool socket_base::is_connected() const
{
  return m_is_connected && m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed;
}

/**
 * @brief Check if socket is closed
 * @return True if socket is closed
 */
bool socket_base::is_closed() const
{
  return m_is_closed || m_sockfd == INVALID_SOCKET_VALUE;
}

/**
 * @brief Poll socket for activity
 * @param timeout Poll timeout
 * @param mode Poll mode (SELECT_READ, SELECT_WRITE, SELECT_ERROR)
 * @return True if socket is ready
 * @throws std::system_error
 */
bool socket_base::poll(const std::chrono::milliseconds &timeout, int mode)
{
  check_initialized();
  try
  {
#ifdef _WIN32
    fd_set read_fds, write_fds, except_fds;
    fd_set *p_read_fds   = nullptr;
    fd_set *p_write_fds  = nullptr;
    fd_set *p_except_fds = nullptr;

    if (mode & SELECT_READ)
    {
      FD_ZERO(&read_fds);
      FD_SET(m_sockfd, &read_fds);
      p_read_fds = &read_fds;
    }

    if (mode & SELECT_WRITE)
    {
      FD_ZERO(&write_fds);
      FD_SET(m_sockfd, &write_fds);
      p_write_fds = &write_fds;
    }

    if (mode & SELECT_ERROR)
    {
      FD_ZERO(&except_fds);
      FD_SET(m_sockfd, &except_fds);
      p_except_fds = &except_fds;
    }

    struct timeval tv;
    tv.tv_sec  = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

    int result = ::select(static_cast<int>(m_sockfd) + 1, p_read_fds,
                          p_write_fds, p_except_fds, &tv);

    if (result == SOCKET_ERROR_VALUE)
    {
      error("select");
    }

    return result > 0;
#else
    struct pollfd pfd;
    pfd.fd      = m_sockfd;
    pfd.events  = 0;
    pfd.revents = 0;

    if (mode & SELECT_READ)
    {
      pfd.events |= POLLIN;
    }

    if (mode & SELECT_WRITE)
    {
      pfd.events |= POLLOUT;
    }

    if (mode & SELECT_ERROR)
    {
      pfd.events |= POLLERR;
    }

    int result = ::poll(&pfd, 1, static_cast<int>(timeout.count()));

    if (result < 0)
    {
      if (errno == EINTR)
      {
        return false;
      }
      error("poll");
    }

    return result > 0;
#endif
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Get number of bytes available for reading
 * @return Number of bytes available
 * @throws std::system_error
 */
int socket_base::available()
{
  check_initialized();
  try
  {
    int result;

#ifdef _WIN32
    u_long bytes_available = 0;
    if (ioctlsocket(m_sockfd, FIONREAD, &bytes_available) != 0)
    {
      error("ioctlsocket");
    }
    result = static_cast<int>(bytes_available);
#else
    if (::ioctl(m_sockfd, FIONREAD, &result) != 0)
    {
      error("ioctl");
    }
#endif

    return result;
  }
  catch (const std::exception &)
  {
    throw;
  }
}

/**
 * @brief Initialize socket with specific address family and type
 * @param family Address family
 * @param type Socket type
 * @throws std::system_error
 */
void socket_base::init(socket_address::Family family, Type type)
{
  if (m_sockfd != INVALID_SOCKET_VALUE)
  {
    throw std::logic_error("Socket already initialized");
  }

  try
  {
    int af          = static_cast<int>(family);
    int socket_type = (type == STREAM_SOCKET)     ? SOCK_STREAM
                      : (type == DATAGRAM_SOCKET) ? SOCK_DGRAM
                                                  : SOCK_RAW;

    init_socket(af, socket_type, 0);
    m_is_closed    = false;
    m_is_connected = false;
  }
  catch (const std::exception &)
  {
    m_sockfd       = INVALID_SOCKET_VALUE;
    m_is_closed    = true;
    m_is_connected = false;
    throw;
  }
}

/**
 * @brief Initialize socket with address family
 * @param af Address family (AF_INET, AF_INET6)
 * @throws std::system_error
 */
void socket_base::init(int af) { init_socket(af, SOCK_STREAM, 0); }

/**
 * @brief Initialize socket with full parameters
 * @param af Address family
 * @param type Socket type
 * @param proto Protocol (default 0)
 * @throws std::system_error
 */
void socket_base::init_socket(int af, int type, int proto)
{
  if (m_sockfd != INVALID_SOCKET_VALUE)
  {
    throw std::logic_error("Socket already initialized");
  }

  m_sockfd = ::socket(af, type, proto);

  if (m_sockfd == INVALID_SOCKET_VALUE)
  {
    error("socket");
  }
}

/**
 * @brief Check if socket is initialized and not closed
 * @throws std::logic_error if socket is not initialized or is closed
 */
void socket_base::check_initialized() const
{
  if (m_sockfd == INVALID_SOCKET_VALUE)
  {
    throw std::logic_error("Socket not initialized");
  }
  if (m_is_closed)
  {
    throw std::logic_error("Socket is closed");
  }
}

/**
 * @brief Reset socket descriptor to a new value
 * @param fd New socket descriptor to assign (default INVALID_SOCKET_VALUE)
 * @throws std::logic_error if socket is already initialized
 */
void socket_base::reset(socket_t fd)
{
  if (m_sockfd != INVALID_SOCKET_VALUE)
  {
    throw std::logic_error("Socket already initialized");
  }

  m_sockfd = fd;
}

/**
 * @brief Perform ioctl operation with integer argument
 * @param request ioctl request code
 * @param arg Reference to integer argument that will be modified
 * @throws std::system_error on failure
 */
void socket_base::ioctl(unsigned long request, int &arg)
{
#ifdef _WIN32
  if (ioctlsocket(m_sockfd, request, reinterpret_cast<u_long *>(&arg)) != 0)
  {
    error("ioctlsocket");
  }
#else
  if (::ioctl(m_sockfd, request, &arg) != 0)
  {
    error("ioctl");
  }
#endif
}

/**
 * @brief Perform ioctl operation with pointer argument
 * @param request ioctl request code
 * @param arg Pointer to argument data
 * @throws std::system_error on failure
 */
void socket_base::ioctl(unsigned long request, void *arg)
{
#ifdef _WIN32
  if (ioctlsocket(m_sockfd, request, static_cast<u_long *>(arg)) != 0)
  {
    error("ioctlsocket");
  }
#else
  if (::ioctl(m_sockfd, request, arg) != 0)
  {
    error("ioctl");
  }
#endif
}

/**
 * @brief Check for broken timeout implementation and apply workaround
 * @param mode Select mode (currently unused, reserved for future platform-specific fixes)
 */
void socket_base::check_broken_timeout(SelectMode /* mode */)
{
  // Implementation placeholder for broken timeout handling
  // This would be used to work around platform-specific timeout issues
}

/**
 * @brief Get description of the last socket error
 * @return Human-readable description of the last error that occurred
 */
std::string socket_base::last_error_desc() { return describe_native_error(last_error()); }

/**
 * @brief Throw exception based on the last socket error
 * @throws std::system_error with error code and message
 */
void socket_base::error()
{
  const int code = last_error();
  throw std::system_error(make_error_code_from_native(code),
                          compose_error_message({}, code));
}

/**
 * @brief Throw exception with custom message based on the last socket error
 * @param arg Additional context message to prepend to the error description
 * @throws std::system_error with error code and composed message
 */
void socket_base::error(const std::string &arg)
{
  const int code = last_error();
  throw std::system_error(make_error_code_from_native(code),
                          compose_error_message(arg, code));
}

/**
 * @brief Throw exception for a specific error code
 * @param code Native error code to convert and throw
 * @throws std::system_error with the specified error code
 */
void socket_base::error(int code)
{
  throw std::system_error(make_error_code_from_native(code),
                          compose_error_message({}, code));
}

/**
 * @brief Throw exception for a specific error code with custom message
 * @param code Native error code to convert and throw
 * @param arg Additional context message to prepend to the error description
 * @throws std::system_error with the specified error code and composed message
 */
void socket_base::error(int code, const std::string &arg)
{
  throw std::system_error(make_error_code_from_native(code),
                          compose_error_message(arg, code));
}

#ifdef _WIN32
/**
 * @brief Initialize Winsock library (Windows only)
 * @throws std::runtime_error if Winsock initialization fails
 */
void socket_base::init_winsock()
{
  if (!s_winsock_initialized)
  {
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0)
    {
      throw std::runtime_error("Failed to initialize Winsock");
    }
    s_winsock_initialized = true;
  }
  ++s_winsock_ref_count;
}

/**
 * @brief Cleanup Winsock library (Windows only)
 * Decrements reference count and cleans up Winsock when count reaches zero
 */
void socket_base::cleanup_winsock()
{
  if (s_winsock_initialized && --s_winsock_ref_count == 0)
  {
    WSACleanup();
    s_winsock_initialized = false;
  }
}
#else
/**
 * @brief Initialize Winsock library (no-op on non-Windows platforms)
 */
void socket_base::init_winsock()
{
  // No-op on non-Windows platforms
}

/**
 * @brief Cleanup Winsock library (no-op on non-Windows platforms)
 */
void socket_base::cleanup_winsock()
{
  // No-op on non-Windows platforms
}
#endif

} // namespace fb
