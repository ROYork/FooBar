#pragma once

#include <fb/socket_base.h>
#include <vector>
#include <map>
#include <chrono>


#ifdef __linux__
#include <sys/epoll.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#else
// Fallback to select
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif
#endif


namespace fb {

/**
 * @brief Socket event information returned by poll operation
 */
struct SocketEvent
{
  socket_base* socket_ptr =nullptr; ///< Pointer to the socket that has events
  int mode       =0;          ///< Event mode flags (combination of Mode values)

  SocketEvent(socket_base* sock, int event_mode) :
    socket_ptr(sock),
    mode(event_mode)
  {}
};


/**
 * @brief Socket polling and multiplexing for high-performance applications.
 */
class poll_set
{
public:

  /**
   * @brief Polling modes for socket events
   */
  enum Mode : int
  {
    POLL_READ  = 1,     ///< Monitor for read availability
    POLL_WRITE = 2,     ///< Monitor for write availability
    POLL_ERROR = 4      ///< Monitor for error conditions
  };


  /// @brief Default constructor - creates empty poll set
  poll_set();

  /// @brief Destructor - automatically closes platform resources
  ~poll_set();

  poll_set(const poll_set&)            = delete;
  poll_set& operator=(const poll_set&) = delete;
  poll_set(poll_set&& other) noexcept;
  poll_set& operator=(poll_set&& other) noexcept;

  void add(const socket_base& socket, int mode);
  void remove(const socket_base& socket);
  void update(const socket_base& socket, int mode);
  bool has(const socket_base& socket) const;
  int  get_mode(const socket_base& socket) const;
  void clear();
  bool empty() const;
  int  poll(const std::chrono::milliseconds& timeout);
  void clear_events();
  std::size_t count() const;

  // Platform information
  static const char* polling_method();
  static bool efficient_polling();

  int poll(std::vector<SocketEvent>& events, const std::chrono::milliseconds& timeout);
  const std::vector<SocketEvent>& events() const;

private:

  // Socket tracking
  struct SocketInfo
  {
    socket_t fd        = INVALID_SOCKET_VALUE;  ///< socket file descriptor
    socket_base* socket_ptr = nullptr;          ///< Pointer to socket_base object
    int mode           = 0;                     ///< Current polling mode

    SocketInfo() = default;
    SocketInfo(socket_t socket_fd, socket_base* sock, int poll_mode):
      fd(socket_fd),
      socket_ptr(sock),
      mode(poll_mode)
    {}
  };

  std::map<socket_t, SocketInfo> m_sockets;  ///< Map of socket fd to info
  std::vector<SocketEvent> m_events;                   ///< Events from last poll

#ifdef __linux__
  // Linux epoll implementation
  int m_epoll_fd;                            ///< epoll file descriptor
  std::vector<struct epoll_event> m_epoll_events; ///< epoll event buffer

  void init_epoll();
  void cleanup_epoll();
  int poll_epoll(const std::chrono::milliseconds& timeout);
  int epoll_mode_to_events(int mode) const;
  int epoll_events_to_mode(uint32_t events) const;

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
  // BSD kqueue implementation
  int m_kqueue_fd;                           ///< kqueue file descriptor
  std::vector<struct kevent> m_kevent_list;  ///< kevent change list
  std::vector<struct kevent> m_kevent_events; ///< kevent event buffer

  void init_kqueue();
  void cleanup_kqueue();
  int poll_kqueue(const std::chrono::milliseconds& timeout);
  void add_kevent(socket_t fd, int mode);
  void remove_kevent(socket_t fd, int old_mode);
  int kevent_to_mode(const struct kevent& event) const;

#else
  // Fallback select implementation
  fd_set m_read_fds;                         ///< Read file descriptor set
  fd_set m_write_fds;                        ///< Write file descriptor set
  fd_set m_error_fds;                        ///< Error file descriptor set
  socket_t m_max_fd;                         ///< Highest file descriptor number

  void init_select();
  void cleanup_select();
  int poll_select(const std::chrono::milliseconds& timeout);
  void rebuild_fd_sets();
#endif


  void init_polling();
  void cleanup_polling();

  socket_t get_socket_fd(const socket_base& socket) const;

  std::map<socket_t, SocketInfo>::iterator find_socket(const socket_base& socket);
  std::map<socket_t, SocketInfo>::const_iterator find_socket(const socket_base& socket) const;
};

} // namespace fb
