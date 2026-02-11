#pragma once

#include <fb/socket_address.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socket_t = SOCKET;
  using socklen_t = int;
  constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
  constexpr int SOCKET_ERROR_VALUE = SOCKET_ERROR;
#else
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  using socket_t = int;
  constexpr socket_t INVALID_SOCKET_VALUE = -1;
  constexpr int SOCKET_ERROR_VALUE = -1;
#endif

namespace fb {

/**
 * @brief Base socket wrapper class
 * This class provides a clean, high-level interface for socket operations.
 * 
 * @note Named socket_base to avoid naming conflicts with system socket() function.
 */
class socket_base
{
  friend class poll_set;  // Allow poll_set to access socket descriptor

public:

  /**
   * @brief socket types supported by the library
   */
  enum Type
  {
      STREAM_SOCKET = SOCK_STREAM,
      DATAGRAM_SOCKET = SOCK_DGRAM,
      RAW_SOCKET = SOCK_RAW
  };

  /**
   * @brief Select modes for polling operations
   */
  enum SelectMode : int
  {
      SELECT_READ = 1,
      SELECT_WRITE = 2,
      SELECT_ERROR = 4
  };

  socket_base();
  socket_base(socket_address::Family family, Type type);
  explicit socket_base(socket_t sockfd);
  socket_base(const socket_base&) = delete;
  socket_base(socket_base&& other) noexcept;
  virtual ~socket_base();

  socket_base& operator=(const socket_base&) = delete;
  socket_base& operator=(socket_base&& other) noexcept;

  void close();
  void bind(const socket_address& address, bool reuse_address = true);
  void bind(const socket_address& address, bool reuse_address, bool reuse_port);
  void listen(int backlog = 64);
  void connect(const socket_address& address);
  void connect(const socket_address& address, const std::chrono::milliseconds& timeout);
  socket_base accept_connection(socket_address& client_address);
  socket_base accept_connection();

  void set_option(int level, int option, int value);
  void set_option(int level, int option, unsigned int value);
  void set_option(int level, int option, unsigned char value);

  void get_option(int level, int option, int& value);
  void get_option(int level, int option, unsigned int& value);
  void get_option(int level, int option, unsigned char& value);

  void set_linger(bool on, int seconds);
  void get_linger(bool& on, int& seconds);
  void set_no_delay(bool flag);
  bool get_no_delay();
  void set_keep_alive(bool flag);
  bool get_keep_alive();
  void set_reuse_address(bool flag);
  bool get_reuse_address();
  void set_reuse_port(bool flag);
  bool get_reuse_port();
  void set_broadcast(bool flag);
  bool get_broadcast();

  void set_blocking(bool flag);
  bool get_blocking() const;


  void set_send_buffer_size(int size);
  int get_send_buffer_size();

  void set_receive_buffer_size(int size);
  int get_receive_buffer_size();

  void set_send_timeout(const std::chrono::milliseconds& timeout);
  std::chrono::milliseconds get_send_timeout();
  void set_receive_timeout(const std::chrono::milliseconds& timeout);
  std::chrono::milliseconds get_receive_timeout();

  socket_address address() const;
  socket_address peer_address() const;

  // State queries

  bool is_connected() const;
  bool is_closed() const;

  virtual bool secure() const;

  bool poll(const std::chrono::milliseconds& timeout, int mode);

  int available();

  static bool supports_ipv4();
  static bool supports_ipv6();

  static int last_error();
  static std::string last_error_desc();
  static void error();
  static void error(const std::string& arg);
  static void error(int code);
  static void error(int code, const std::string& arg);

protected:

  void init(socket_address::Family family, Type type);
  virtual void init(int af);
  void init_socket(int af, int type, int proto = 0);
  void check_initialized() const;
  socket_t sockfd() const;
  bool initialized() const;
  void reset(socket_t fd = INVALID_SOCKET_VALUE);
  void ioctl(unsigned long request, int& arg);
  void ioctl(unsigned long request, void* arg);
  void set_connected(bool connected);  ///< Update connection state (for derived classes)

private:

  void check_broken_timeout(SelectMode mode);
  static void init_winsock();

  static void cleanup_winsock();

  // socket descriptor and state
  socket_t m_sockfd;
  bool m_is_connected;
  bool m_is_closed;

  // Timeout settings
  std::chrono::milliseconds m_recv_timeout;
  std::chrono::milliseconds m_send_timeout;

  // socket options
  bool m_blocking;
  bool m_is_broken_timeout;  // Platform-specific timeout handling flag

#ifdef _WIN32
  // Windows Winsock initialization tracking
  static bool s_winsock_initialized;
  static int s_winsock_ref_count;
#endif
};

// Inline implementations

/**
 * @brief Get Socket File descriptor
 */
inline socket_t socket_base::sockfd() const
{
  return m_sockfd;
}


inline bool socket_base::initialized() const
{
    return m_sockfd != INVALID_SOCKET_VALUE;
}

/**
 * @brief Get blocking mode
 * @return True if socket is in blocking mode
 */
inline bool socket_base::get_blocking() const
{
  return m_blocking;
}

inline bool socket_base::secure() const
{
    return false;
}

inline bool socket_base::supports_ipv4()
{
    return true;
}

inline bool socket_base::supports_ipv6()
{
#ifdef AF_INET6
    return true;
#else
    return false;
#endif
}

inline int socket_base::last_error()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

} // namespace fb
