#include <fb/poll_set.h>
#include <fb/detail/socket_error_utils.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef __linux__
#include <sys/epoll.h>
#include <unistd.h>
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
#include <sys/event.h>
#include <sys/types.h>
#include <unistd.h>
#else
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif
#endif

namespace fb
{

/**
 * @class fb::poll_set
 * @brief Cross-platform wrapper for scalable socket polling mechanisms.
 *
 * The implementation selects the most efficient OS facility available
 * (epoll, kqueue, or select) and exposes a uniform interface for monitoring
 * multiple sockets for readability, writability, and error conditions.
 */

// Platform-specific constants
#ifdef __linux__
constexpr int INITIAL_EPOLL_EVENTS = 64;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
constexpr int INITIAL_KEVENT_SIZE = 64;
#endif

/**
 * @brief Construct an empty poll set and initialize OS resources.
 */
poll_set::poll_set() { init_polling(); }

/**
 * @brief Destroy the poll set and release OS resources.
 */
poll_set::~poll_set() { cleanup_polling(); }

/**
 * @brief Move construct a poll set and transfer ownership of resources.
 */
poll_set::poll_set(poll_set &&other) noexcept :
  m_sockets(std::move(other.m_sockets)),
  m_events(std::move(other.m_events))
{
#ifdef __linux__
  m_epoll_fd       = other.m_epoll_fd;
  m_epoll_events   = std::move(other.m_epoll_events);
  other.m_epoll_fd = -1;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  m_kqueue_fd       = other.m_kqueue_fd;
  m_kevent_list     = std::move(other.m_kevent_list);
  m_kevent_events   = std::move(other.m_kevent_events);
  other.m_kqueue_fd = -1;
#else
  m_read_fds  = other.m_read_fds;
  m_write_fds = other.m_write_fds;
  m_error_fds = other.m_error_fds;
  m_max_fd    = other.m_max_fd;
#endif
}

/**
 * @brief Move-assign a poll set, releasing current resources first.
 *
 * @return Reference to this instance.
 */
poll_set &poll_set::operator=(poll_set &&other) noexcept
{
  if (this != &other)
  {
    cleanup_polling();

    m_sockets = std::move(other.m_sockets);
    m_events  = std::move(other.m_events);

#ifdef __linux__
    m_epoll_fd       = other.m_epoll_fd;
    m_epoll_events   = std::move(other.m_epoll_events);
    other.m_epoll_fd = -1;
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
    m_kqueue_fd       = other.m_kqueue_fd;
    m_kevent_list     = std::move(other.m_kevent_list);
    m_kevent_events   = std::move(other.m_kevent_events);
    other.m_kqueue_fd = -1;
#else
    m_read_fds  = other.m_read_fds;
    m_write_fds = other.m_write_fds;
    m_error_fds = other.m_error_fds;
    m_max_fd    = other.m_max_fd;
#endif
  }
  return *this;
}

/**
 * @brief Add socket to poll set with specified mode
 * @param socket Socket to monitor
 * @param mode Polling mode (POLL_READ, POLL_WRITE, POLL_ERROR or combination)
 * @throws std::system_error on error
 */
void poll_set::add(const socket_base &socket, int mode)
{
  socket_t fd = get_socket_fd(socket);
  if (fd == -1)
  {
    throw std::invalid_argument("Invalid socket file descriptor");
  }

  auto it = m_sockets.find(fd);
  if (it != m_sockets.end())
  {
    // Socket already exists, update mode instead
    update(socket, mode);
    return;
  }

  // Add new socket
  // SocketInfo info(fd, const_cast<socket*>(&socket), mode);
  SocketInfo info(fd, const_cast<class socket_base *>(&socket), mode);
  m_sockets[fd] = info;

#ifdef __linux__
  struct epoll_event event;
  event.events  = epoll_mode_to_events(mode);
  event.data.fd = fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
  {
    m_sockets.erase(fd);
    detail::throw_system_error(errno, "Failed to add socket to epoll");
  }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  add_kevent(fd, mode);
#else
  rebuild_fd_sets();
#endif
}

/**
 * @brief Remove socket from poll set
 * @param socket Socket to remove
 * @throws std::system_error on error
 */
void poll_set::remove(const socket_base &socket)
{
  auto it = find_socket(socket);
  if (it == m_sockets.end())
  {
    return; // Socket not in set
  }

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__NetBSD__) || defined(__OpenBSD__)
  socket_t fd = it->first;
#endif

#ifdef __linux__
  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1)
  {
    detail::throw_system_error(errno, "Failed to remove socket from epoll");
  }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  int old_mode = it->second.mode;
  remove_kevent(fd, old_mode);
#endif

  m_sockets.erase(it);

#if !defined(__linux__) && !defined(__APPLE__) && !defined(__FreeBSD__) &&     \
    !defined(__NetBSD__) && !defined(__OpenBSD__)
  rebuild_fd_sets();
#endif
}

/**
 * @brief Update socket polling mode
 * @param socket Socket to update
 * @param mode New polling mode
 * @throws std::system_error on error
 */
void poll_set::update(const socket_base &socket, int mode)
{
  auto it = find_socket(socket);
  if (it == m_sockets.end())
  {
    throw std::logic_error("Socket not in poll set");
  }

  socket_t fd  = it->first;
  int old_mode = it->second.mode;

  if (old_mode == mode)
  {
    return; // No change needed
  }

  it->second.mode = mode;

#if defined(_WIN32)
  // On Windows, suppress unused variable warnings for platform-specific code
  (void)fd;
#endif

#ifdef __linux__
  struct epoll_event event;
  event.events  = epoll_mode_to_events(mode);
  event.data.fd = fd;

  if (epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &event) == -1)
  {
    it->second.mode = old_mode; // Revert change
    detail::throw_system_error(errno, "Failed to update socket in epoll");
  }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  remove_kevent(fd, old_mode);
  add_kevent(fd, mode);
#else
  rebuild_fd_sets();
#endif
}

/**
 * @brief Check if socket is in the poll set
 * @param socket Socket to check
 * @return True if socket is being monitored
 */
bool poll_set::has(const socket_base &socket) const
{
  return find_socket(socket) != m_sockets.end();
}

/**
 * @brief Get current polling mode for a socket
 * @param socket Socket to query
 * @return Current polling mode, 0 if socket not in set
 */
int poll_set::get_mode(const socket_base &socket) const
{
  auto it = find_socket(socket);
  return (it != m_sockets.end()) ? it->second.mode : 0;
}

/**
 * @brief Remove all sockets from poll set
 */
void poll_set::clear()
{
  m_sockets.clear();
  m_events.clear();

#ifdef __linux__
  // Close and recreate epoll instance for clean slate
  cleanup_epoll();
  init_epoll();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  // Clear kevent lists
  m_kevent_list.clear();
  m_kevent_events.clear();
  m_kevent_events.resize(INITIAL_KEVENT_SIZE);
#else
  FD_ZERO(&m_read_fds);
  FD_ZERO(&m_write_fds);
  FD_ZERO(&m_error_fds);
  m_max_fd = INVALID_SOCKET_VALUE;
#endif
}

/**
 * @brief Get number of sockets in poll set
 * @return Number of monitored sockets
 */
std::size_t poll_set::count() const { return m_sockets.size(); }

/**
 * @brief Check if poll set is empty
 * @return True if no sockets are being monitored
 */
bool poll_set::empty() const { return m_sockets.empty(); }

/**
 * @brief Poll sockets for events with timeout
 * @param timeout Maximum time to wait for events (negative = infinite)
 * @return Number of sockets with events
 * @throws std::system_error on error
 */
int poll_set::poll(const std::chrono::milliseconds &timeout)
{
  m_events.clear();

#ifdef __linux__
  return poll_epoll(timeout);
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  return poll_kqueue(timeout);
#else
  return poll_select(timeout);
#endif
}

/**
 * @brief Poll sockets for events with timeout, storing results in vector
 * @param events Vector to store socket events (will be cleared first)
 * @param timeout Maximum time to wait for events (negative = infinite)
 * @return Number of sockets with events
 * @throws std::system_error on error
 */
int poll_set::poll(std::vector<SocketEvent> &events,
                   const std::chrono::milliseconds &timeout)
{
  int result = poll(timeout);
  events     = m_events;
  return result;
}

/**
 * @brief Get events from last poll operation
 * @return Vector of socket events from last poll
 */
const std::vector<SocketEvent> &poll_set::events() const { return m_events; }

/**
 * @brief Clear stored events from last poll operation
 */
void poll_set::clear_events() { m_events.clear(); }

/**
 * @brief Get polling mechanism name
 * @return String describing the polling method used ("epoll", "kqueue",
 * "select")
 */
const char *poll_set::polling_method()
{
#ifdef __linux__
  return "epoll";
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  return "kqueue";
#else
  return "select";
#endif
}

/**
 * @brief Check if this platform supports efficient polling
 * @return True for epoll/kqueue, false for select fallback
 */
bool poll_set::efficient_polling()
{
#ifdef __linux__
  return true; // epoll is efficient
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  return true; // kqueue is efficient
#else
  return false; // select is less efficient
#endif
}

/**
 * @brief Get socket file descriptor safely
 * @param socket Socket object
 * @return File descriptor or -1 on error
 */
socket_t poll_set::get_socket_fd(const socket_base &socket) const
{
  try
  {
    return socket.sockfd();
  }
  catch (...)
  {
    return static_cast<socket_t>(-1);
  }
}

/**
 * @brief Find socket info by Socket pointer
 * @param socket Socket to find
 * @return Iterator to socket info, or end() if not found
 */
/**
 * @brief Find socket info by Socket pointer
 * @param socket Socket to find
 * @return Iterator to socket info, or end() if not found
 */
std::map<socket_t, poll_set::SocketInfo>::iterator
poll_set::find_socket(const socket_base &socket)
{
  return std::find_if(m_sockets.begin(), m_sockets.end(),
                      [&socket](const auto &pair)
                      { return pair.second.socket_ptr == &socket; });
}

/**
 * @brief Find socket info by Socket pointer
 * @param socket Socket to find
 * @return Iterator to socket info, or end() if not found
 */
/**
 * @brief Find socket info by socket pointer
 * @param socket Socket to find
 * @return Iterator to socket info, or end() if not found
 */
std::map<socket_t, poll_set::SocketInfo>::const_iterator
poll_set::find_socket(const socket_base &socket) const
{
  return std::find_if(m_sockets.begin(), m_sockets.end(),
                      [&socket](const auto &pair)
                      { return pair.second.socket_ptr == &socket; });
}

/**
 * @brief Initialize platform-specific polling mechanism
 */
void poll_set::init_polling()
{
#ifdef __linux__
  init_epoll();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  init_kqueue();
#else
  init_select();
#endif
}

/**
 * @brief Cleanup platform-specific polling resources
 */
/**
 * @brief Cleanup platform-specific polling resources
 */
void poll_set::cleanup_polling()
{
#ifdef __linux__
  cleanup_epoll();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)
  cleanup_kqueue();
#else
  cleanup_select();
#endif
}

#ifdef __linux__

void poll_set::init_epoll()
{
  m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (m_epoll_fd == -1)
  {
    detail::throw_system_error(errno, "Failed to create epoll instance");
  }
  m_epoll_events.resize(INITIAL_EPOLL_EVENTS);
}

void poll_set::cleanup_epoll()
{
  if (m_epoll_fd != -1)
  {
    close(m_epoll_fd);
    m_epoll_fd = -1;
  }
}

int poll_set::poll_epoll(const std::chrono::milliseconds &timeout)
{
  if (m_sockets.empty())
  {
    return 0;
  }

  int timeout_ms = timeout.count() < 0 ? -1 : static_cast<int>(timeout.count());

  int num_events =
      epoll_wait(m_epoll_fd, m_epoll_events.data(),
                 static_cast<int>(m_epoll_events.size()), timeout_ms);

  if (num_events == -1)
  {
    if (errno == EINTR)
    {
      return 0; // Interrupted, but not an error
    }
    detail::throw_system_error(errno, "epoll_wait failed");
  }

  m_events.clear();
  m_events.reserve(num_events);

  for (int i = 0; i < num_events; ++i)
  {
    socket_t fd = m_epoll_events[i].data.fd;
    auto it     = m_sockets.find(fd);
    if (it != m_sockets.end())
    {
      int mode = epoll_events_to_mode(m_epoll_events[i].events);
      m_events.emplace_back(it->second.socket_ptr, mode);
    }
  }

  // Resize event buffer if we filled it completely
  if (num_events == static_cast<int>(m_epoll_events.size()))
  {
    m_epoll_events.resize(m_epoll_events.size() * 2);
  }

  return num_events;
}

int poll_set::epoll_mode_to_events(int mode) const
{
  int events = 0;
  if (mode & POLL_READ)
  {
    events |= EPOLLIN;
  }
  if (mode & POLL_WRITE)
  {
    events |= EPOLLOUT;
  }
  if (mode & POLL_ERROR)
  {
    events |= EPOLLERR | EPOLLHUP;
  }
  return events;
}

int poll_set::epoll_events_to_mode(uint32_t events) const
{
  int mode = 0;
  if (events & EPOLLIN)
  {
    mode |= POLL_READ;
  }
  if (events & EPOLLOUT)
  {
    mode |= POLL_WRITE;
  }
  if (events & (EPOLLERR | EPOLLHUP))
  {
    mode |= POLL_ERROR;
  }
  return mode;
}

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) ||     \
    defined(__OpenBSD__)

// BSD kqueue implementation
void poll_set::init_kqueue()
{
  m_kqueue_fd = kqueue();
  if (m_kqueue_fd == -1)
  {
    detail::throw_system_error(errno, "Failed to create kqueue instance");
  }
  m_kevent_events.resize(INITIAL_KEVENT_SIZE);
}

void poll_set::cleanup_kqueue()
{
  if (m_kqueue_fd != -1)
  {
    close(m_kqueue_fd);
    m_kqueue_fd = -1;
  }
}

int poll_set::poll_kqueue(const std::chrono::milliseconds &timeout)
{
  if (m_sockets.empty())
  {
    return 0;
  }

  struct timespec ts;
  struct timespec *timeout_ptr = nullptr;

  if (timeout.count() >= 0)
  {
    ts.tv_sec   = timeout.count() / 1000;
    ts.tv_nsec  = (timeout.count() % 1000) * 1000000;
    timeout_ptr = &ts;
  }

  int num_events = kevent(
      m_kqueue_fd, m_kevent_list.empty() ? nullptr : m_kevent_list.data(),
      static_cast<int>(m_kevent_list.size()), m_kevent_events.data(),
      static_cast<int>(m_kevent_events.size()), timeout_ptr);

  // Clear change list after applying changes
  m_kevent_list.clear();

  if (num_events == -1)
  {
    if (errno == EINTR)
    {
      return 0; // Interrupted, but not an error
    }
    detail::throw_system_error(errno, "kevent failed");
  }

  m_events.clear();
  m_events.reserve(num_events);

  for (int i = 0; i < num_events; ++i)
  {
    socket_t fd = static_cast<socket_t>(m_kevent_events[i].ident);
    auto it     = m_sockets.find(fd);
    if (it != m_sockets.end())
    {
      int mode = kevent_to_mode(m_kevent_events[i]);
      m_events.emplace_back(it->second.socket_ptr, mode);
    }
  }

  // Resize event buffer if needed
  if (num_events == static_cast<int>(m_kevent_events.size()))
  {
    m_kevent_events.resize(m_kevent_events.size() * 2);
  }

  return num_events;
}

void poll_set::add_kevent(socket_t fd, int mode)
{
  if (mode & POLL_READ)
  {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    m_kevent_list.push_back(event);
  }

  if (mode & POLL_WRITE)
  {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    m_kevent_list.push_back(event);
  }
}

void poll_set::remove_kevent(socket_t fd, int old_mode)
{
  if (old_mode & POLL_READ)
  {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    m_kevent_list.push_back(event);
  }
  if (old_mode & POLL_WRITE)
  {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    m_kevent_list.push_back(event);
  }
}

int poll_set::kevent_to_mode(const struct kevent &event) const
{
  int mode = 0;
  if (event.filter == EVFILT_READ)
  {
    mode |= POLL_READ;
    if (event.flags & EV_EOF)
    {
      mode |= POLL_ERROR;
    }
  }
  else if (event.filter == EVFILT_WRITE)
  {
    mode |= POLL_WRITE;
    if (event.flags & EV_EOF)
    {
      mode |= POLL_ERROR;
    }
  }
  if (event.flags & EV_ERROR)
  {
    mode |= POLL_ERROR;
  }

  return mode;
}

#else

// Fallback select implementation
void poll_set::init_select()
{
  FD_ZERO(&m_read_fds);
  FD_ZERO(&m_write_fds);
  FD_ZERO(&m_error_fds);
  m_max_fd = INVALID_SOCKET_VALUE;
}

void poll_set::cleanup_select()
{
  // Nothing to clean up for select
}

int poll_set::poll_select(const std::chrono::milliseconds &timeout)
{
  if (m_sockets.empty())
  {
    return 0;
  }

  fd_set read_fds  = m_read_fds;
  fd_set write_fds = m_write_fds;
  fd_set error_fds = m_error_fds;

  struct timeval tv;
  struct timeval *timeout_ptr = nullptr;

  if (timeout.count() >= 0)
  {
    tv.tv_sec   = static_cast<long>(timeout.count() / 1000);
    tv.tv_usec  = static_cast<long>((timeout.count() % 1000) * 1000);
    timeout_ptr = &tv;
  }

  int result = select(static_cast<int>(m_max_fd + 1), &read_fds, &write_fds,
                      &error_fds, timeout_ptr);

  if (result == -1)
  {
#ifdef _WIN32
    if (WSAGetLastError() == WSAEINTR)
    {
      return 0;
    }
    detail::throw_system_error(WSAGetLastError(), "select failed");
#else
    if (errno == EINTR)
    {
      return 0;
    }
    detail::throw_system_error(errno, "select failed");
#endif
  }

  m_events.clear();

  for (const auto &pair : m_sockets)
  {
    socket_t fd            = pair.first;
    const SocketInfo &info = pair.second;
    int mode               = 0;

    if (FD_ISSET(fd, &read_fds))
    {
      mode |= POLL_READ;
    }
    if (FD_ISSET(fd, &write_fds))
    {
      mode |= POLL_WRITE;
    }
    if (FD_ISSET(fd, &error_fds))
    {
      mode |= POLL_ERROR;
    }

    if (mode != 0)
    {
      m_events.emplace_back(info.socket_ptr, mode);
    }
  }

  return static_cast<int>(m_events.size());
}

void poll_set::rebuild_fd_sets()
{
  FD_ZERO(&m_read_fds);
  FD_ZERO(&m_write_fds);
  FD_ZERO(&m_error_fds);
  m_max_fd = INVALID_SOCKET_VALUE;

  for (const auto &pair : m_sockets)
  {
    socket_t fd = pair.first;
    int mode    = pair.second.mode;

    if (mode & POLL_READ)
    {
      FD_SET(fd, &m_read_fds);
    }
    if (mode & POLL_WRITE)
    {
      FD_SET(fd, &m_write_fds);
    }
    if (mode & POLL_ERROR)
    {
      FD_SET(fd, &m_error_fds);
    }
    if (fd > m_max_fd)
    {
      m_max_fd = fd;
    }
  }
}

#endif

} // namespace fb
