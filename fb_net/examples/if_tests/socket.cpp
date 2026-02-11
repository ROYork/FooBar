#include <fb/socket_base.h>
#include <cassert>
#include <utility>
#include <stdexcept>
#include <system_error>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "mswsock.lib")
#else
    #include <sys/socket.h>
    #include <sys/select.h>
    #include <sys/time.h>
    #include <sys/ioctl.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <poll.h>
#endif

namespace fb
{

// Static member initialization
#ifdef _WIN32
bool socket::s_winsock_initialized = false;
int socket::s_winsock_ref_count = 0;
#endif

/**
 * @brief Default constructor creates an uninitialized socket
 */
socket::socket()
    : m_sockfd(INVALID_SOCKET_VALUE)
    , m_is_connected(false)
    , m_is_closed(true)
    , m_recv_timeout(std::chrono::milliseconds(0))
    , m_send_timeout(std::chrono::milliseconds(0))
    , m_blocking(true)
    , m_is_broken_timeout(false)
{
#ifdef _WIN32
    init_winsock();
#endif
}

/**
 * @brief Create a socket with specific address family and type
 * @param family Address family (IPv4, IPv6)
 * @param type socket type (STREAM, DATAGRAM, RAW)
 */
socket::socket(socket_address::Family family, Type type)
    : m_sockfd(INVALID_SOCKET_VALUE)
    , m_is_connected(false)
    , m_is_closed(true)
    , m_recv_timeout(std::chrono::milliseconds(0))
    , m_send_timeout(std::chrono::milliseconds(0))
    , m_blocking(true)
    , m_is_broken_timeout(false)
{
#ifdef _WIN32
    init_winsock();
#endif
    init(family, type);
}

/**
 * @brief Create socket from existing socket descriptor (for accept operations)
 * @param sockfd socket descriptor to wrap
 */
socket::socket(socket_t sockfd)
    : m_sockfd(sockfd)
    , m_is_connected(true)
    , m_is_closed(false)
    , m_recv_timeout(std::chrono::milliseconds(0))
    , m_send_timeout(std::chrono::milliseconds(0))
    , m_blocking(true)
    , m_is_broken_timeout(false)
{
#ifdef _WIN32
    init_winsock();
#endif
}

/**
 * @brief Move constructor
 */
socket::socket(socket&& other) noexcept
    : m_sockfd(other.m_sockfd)
    , m_is_connected(other.m_is_connected)
    , m_is_closed(other.m_is_closed)
    , m_recv_timeout(other.m_recv_timeout)
    , m_send_timeout(other.m_send_timeout)
    , m_blocking(other.m_blocking)
    , m_is_broken_timeout(other.m_is_broken_timeout)
{
    other.m_sockfd = INVALID_SOCKET_VALUE;
    other.m_is_connected = false;
    other.m_is_closed = true;
}

/**
 * @brief Move assignment operator
 */
socket& socket::operator=(socket&& other) noexcept
{
    if (this != &other) {
        // Close current socket if open
        if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed) {
            try {
                close();
            } catch (...) {
                // Ignore exceptions during move assignment cleanup
            }
        }

        // Move from other
        m_sockfd = other.m_sockfd;
        m_is_connected = other.m_is_connected;
        m_is_closed = other.m_is_closed;
        m_recv_timeout = other.m_recv_timeout;
        m_send_timeout = other.m_send_timeout;
        m_blocking = other.m_blocking;
        m_is_broken_timeout = other.m_is_broken_timeout;

        // Reset other
        other.m_sockfd = INVALID_SOCKET_VALUE;
        other.m_is_connected = false;
        other.m_is_closed = true;
    }
    return *this;
}

/**
 * @brief Destructor - automatically closes socket if open
 */
socket::~socket()
{
    if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed) {
        try {
            close();
        } catch (...) {
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
void socket::close()
{
    if (m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed) {
        try {
#ifdef _WIN32
            closesocket(m_sockfd);
#else
            ::close(m_sockfd);
#endif
            m_sockfd = INVALID_SOCKET_VALUE;
            m_is_connected = false;
            m_is_closed = true;
        } catch (const std::exception&) {
            m_is_connected = false;
            m_is_closed = true;
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
void socket::bind(const socket_address& address, bool reuse_address)
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
void socket::bind(const socket_address& address, bool reuse_address, bool reuse_port)
{
    check_initialized();
    try {
        if (m_sockfd == INVALID_SOCKET_VALUE) {
            init(address.af());
        }

        if (reuse_address) {
            set_reuse_address(true);
        }

        if (reuse_port) {
            set_reuse_port(true);
        }

        int result = ::bind(m_sockfd, address.addr(), address.length());

        if (result != 0) {
            error("bind");
        }
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Start listening for connections (server sockets)
 * @param backlog Maximum number of pending connections
 * @throws std::system_error
 */
void socket::listen(int backlog)
{
    check_initialized();
    try {
        int result = ::listen(m_sockfd, backlog);

        if (result != 0) {
            error("listen");
        }
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Connect to remote address (client sockets)
 * @param address Remote address to connect to
 * @throws std::system_error on configuration failure
 */
void socket::connect(const socket_address& address)
{
    check_initialized();
    try {
        if (m_sockfd == INVALID_SOCKET_VALUE) {
            init(address.af());
        }

        int result = ::connect(m_sockfd, address.addr(), address.length());

        if (result != 0) {
            error("connect");
        }

        m_is_connected = true;
        m_is_closed = false;
    } catch (const std::exception&) {
        m_is_connected = false;
        throw;
    }
}

/**
 * @brief Connect to remote address with timeout
 * @param address Remote address to connect to
 * @param timeout Connection timeout
 * @throws std::system_error on configuration failure
 */
void socket::connect(const socket_address& address, const std::chrono::milliseconds& timeout)
{
    check_initialized();
    try {
        if (m_sockfd == INVALID_SOCKET_VALUE) {
            init(address.af());
        }

        set_blocking(false);

        int result = ::connect(m_sockfd, address.addr(), address.length());

        if (result == 0) {
            set_blocking(true);
            m_is_connected = true;
            m_is_closed = false;
            return;
        }

#ifdef _WIN32
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            set_blocking(true);
            error("connect");
        }
#else
        if (errno != EINPROGRESS) {
            set_blocking(true);
            error("connect");
        }
#endif

        if (!poll(timeout, SELECT_WRITE)) {
            set_blocking(true);
            throw std::runtime_error("Connection timeout");
        }

        int opt_val;
        socklen_t opt_len = sizeof(opt_val);
        if (getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&opt_val), &opt_len) < 0) {
            set_blocking(true);
            error("getsockopt");
        }

        if (opt_val != 0) {
            set_blocking(true);
            error(opt_val, "connect");
        }

        set_blocking(true);
        m_is_connected = true;
        m_is_closed = false;
    } catch (const std::exception&) {
        m_is_connected = false;
        throw;
    }
}

/**
 * @brief Accept incoming connection (server sockets)
 * @param client_address Output parameter for client address
 * @return New socket instance for the connection
 * @throws std::system_error
 */
socket socket::accept_connection(socket_address& client_address)
{
    check_initialized();
    try {
        struct sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);

        socket_t client_fd = ::accept(m_sockfd, reinterpret_cast<struct sockaddr*>(&addr_storage), &addr_len);

        if (client_fd == INVALID_SOCKET_VALUE) {
            error("accept");
        }

        client_address = socket_address(reinterpret_cast<struct sockaddr*>(&addr_storage), addr_len);

        return socket(client_fd);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Accept incoming connection (server sockets)
 * @return New socket instance for the connection
 * @throws std::system_error
 */
socket socket::accept_connection()
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
void socket::set_option(int level, int option, int value)
{
    check_initialized();
    try {
        if (setsockopt(m_sockfd, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) != 0) {
            error("setsockopt");
        }
    } catch (const std::exception&) {
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
void socket::set_option(int level, int option, unsigned int value)
{
    check_initialized();
    try {
        if (setsockopt(m_sockfd, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) != 0) {
            error("setsockopt");
        }
    } catch (const std::exception&) {
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
void socket::set_option(int level, int option, unsigned char value)
{
    check_initialized();
    try {
        if (setsockopt(m_sockfd, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) != 0) {
            error("setsockopt");
        }
    } catch (const std::exception&) {
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
void socket::get_option(int level, int option, int& value)
{
    check_initialized();
    try {
        socklen_t length = sizeof(value);
        if (getsockopt(m_sockfd, level, option, reinterpret_cast<char*>(&value), &length) != 0) {
            error("getsockopt");
        }
    } catch (const std::exception&) {
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
void socket::get_option(int level, int option, unsigned int& value)
{
    check_initialized();
    try {
        socklen_t length = sizeof(value);
        if (getsockopt(m_sockfd, level, option, reinterpret_cast<char*>(&value), &length) != 0) {
            error("getsockopt");
        }
    } catch (const std::exception&) {
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
void socket::get_option(int level, int option, unsigned char& value)
{
    check_initialized();
    try {
        socklen_t length = sizeof(value);
        if (getsockopt(m_sockfd, level, option, reinterpret_cast<char*>(&value), &length) != 0) {
            error("getsockopt");
        }
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set linger option
 * @param on Enable/disable linger
 * @param seconds Linger time in seconds
 * @throws std::system_error
 */
void socket::set_linger(bool on, int seconds)
{
    check_initialized();
    try {
        struct linger l;
        l.l_onoff = on ? 1 : 0;
        l.l_linger = static_cast<u_short>(seconds);
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&l), sizeof(l)) != 0) {
            error("setsockopt");
        }
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get linger option
 * @param on Output parameter for linger enable/disable
 * @param seconds Output parameter for linger time
 * @throws std::system_error
 */
void socket::get_linger(bool& on, int& seconds)
{
    check_initialized();
    try {
        struct linger l;
        socklen_t length = sizeof(l);
        if (getsockopt(m_sockfd, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&l), &length) != 0) {
            error("getsockopt");
        }
        on = l.l_onoff != 0;
        seconds = l.l_linger;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set TCP no delay option (disable Nagle algorithm)
 * @param flag Enable/disable no delay
 * @throws std::system_error
 */
void socket::set_no_delay(bool flag)
{
    check_initialized();
    try {
        int value = flag ? 1 : 0;
        set_option(IPPROTO_TCP, TCP_NODELAY, value);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get TCP no delay option
 * @return True if no delay is enabled
 * @throws std::system_error
 */
bool socket::get_no_delay()
{
    check_initialized();
    try {
        int value;
        get_option(IPPROTO_TCP, TCP_NODELAY, value);
        return value != 0;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set keep alive option
 * @param flag Enable/disable keep alive
 * @throws std::system_error
 */
void socket::set_keep_alive(bool flag)
{
    check_initialized();
    try {
        int value = flag ? 1 : 0;
        set_option(SOL_SOCKET, SO_KEEPALIVE, value);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get keep alive option
 * @return True if keep alive is enabled
 * @throws std::system_error
 */
bool socket::get_keep_alive()
{
    check_initialized();
    try {
        int value;
        get_option(SOL_SOCKET, SO_KEEPALIVE, value);
        return value != 0;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set address reuse option
 * @param flag Enable/disable address reuse
 * @throws std::system_error
 */
void socket::set_reuse_address(bool flag)
{
    check_initialized();
    try {
        int value = flag ? 1 : 0;
        set_option(SOL_SOCKET, SO_REUSEADDR, value);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get address reuse option
 * @return True if address reuse is enabled
 * @throws std::system_error
 */
bool socket::get_reuse_address()
{
    check_initialized();
    try {
        int value;
        get_option(SOL_SOCKET, SO_REUSEADDR, value);
        return value != 0;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set port reuse option
 * @param flag Enable/disable port reuse
 * @throws std::system_error
 */
void socket::set_reuse_port(bool flag)
{
    check_initialized();
    try {
#ifdef SO_REUSEPORT
        int value = flag ? 1 : 0;
        set_option(SOL_SOCKET, SO_REUSEPORT, value);
#endif
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get port reuse option
 * @return True if port reuse is enabled
 * @throws std::system_error
 */
bool socket::get_reuse_port()
{
    check_initialized();
    try {
#ifdef SO_REUSEPORT
        int value;
        get_option(SOL_SOCKET, SO_REUSEPORT, value);
        return value != 0;
#else
        return false;
#endif
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set broadcast option
 * @param flag Enable/disable broadcast
 * @throws std::system_error
 */
void socket::set_broadcast(bool flag)
{
    check_initialized();
    try {
        int value = flag ? 1 : 0;
        set_option(SOL_SOCKET, SO_BROADCAST, value);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get broadcast option
 * @return True if broadcast is enabled
 * @throws std::system_error
 */
bool socket::get_broadcast()
{
    check_initialized();
    try {
        int value;
        get_option(SOL_SOCKET, SO_BROADCAST, value);
        return value != 0;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set blocking mode
 * @param flag True for blocking, false for non-blocking
 * @throws std::system_error
 */
void socket::set_blocking(bool flag)
{
    check_initialized();
    try {
#ifdef _WIN32
        u_long arg = flag ? 0 : 1;
        if (ioctlsocket(m_sockfd, FIONBIO, &arg) != 0) {
            error("ioctlsocket");
        }
#else
        int flags = fcntl(m_sockfd, F_GETFL, 0);
        if (flags == -1) {
            error("fcntl");
        }

        if (flag) {
            flags &= ~O_NONBLOCK;
        }
        else {
            flags |= O_NONBLOCK;
        }

        if (fcntl(m_sockfd, F_SETFL, flags) == -1) {
            error("fcntl");
        }
#endif

        m_blocking = flag;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set send buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void socket::set_send_buffer_size(int size)
{
    check_initialized();
    try {
        set_option(SOL_SOCKET, SO_SNDBUF, size);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get send buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int socket::get_send_buffer_size()
{
    check_initialized();
    try {
        int result;
        get_option(SOL_SOCKET, SO_SNDBUF, result);
        return result;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set receive buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void socket::set_receive_buffer_size(int size)
{
    check_initialized();
    try {
        set_option(SOL_SOCKET, SO_RCVBUF, size);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get receive buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int socket::get_receive_buffer_size()
{
    check_initialized();
    try {
        int result;
        get_option(SOL_SOCKET, SO_RCVBUF, result);
        return result;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set send timeout
 * @param timeout Send timeout
 * @throws std::system_error
 */
void socket::set_send_timeout(const std::chrono::milliseconds& timeout)
{
    check_initialized();
    try {
        m_send_timeout = timeout;

#ifdef _WIN32
        DWORD tv = static_cast<DWORD>(timeout.count());
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)) != 0) {
            error("setsockopt");
        }
#else
        struct timeval tv;
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
            error("setsockopt");
        }
#endif
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get send timeout
 * @return Send timeout
 * @throws std::system_error
 */
std::chrono::milliseconds socket::get_send_timeout()
{
    check_initialized();
    try {
        return m_send_timeout;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Set receive timeout
 * @param timeout Receive timeout
 * @throws std::system_error
 */
void socket::set_receive_timeout(const std::chrono::milliseconds& timeout)
{
    check_initialized();
    try {
        m_recv_timeout = timeout;

#ifdef _WIN32
        DWORD tv = static_cast<DWORD>(timeout.count());
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)) != 0) {
            error("setsockopt");
        }
#else
        struct timeval tv;
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        if (setsockopt(m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
            error("setsockopt");
        }
#endif
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get receive timeout
 * @return Receive timeout
 * @throws std::system_error
 */
std::chrono::milliseconds socket::get_receive_timeout()
{
    check_initialized();
    try {
        return m_recv_timeout;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get local socket address
 * @return Local socket address
 * @throws std::system_error
 */
socket_address socket::address() const
{
    check_initialized();
    try {
        struct sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);

        if (getsockname(m_sockfd, reinterpret_cast<struct sockaddr*>(&addr_storage), &addr_len) != 0) {
            error("getsockname");
        }

        return socket_address(reinterpret_cast<struct sockaddr*>(&addr_storage), addr_len);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get peer socket address (for connected sockets)
 * @return Peer socket address
 * @throws std::system_error
 */
socket_address socket::peer_address() const
{
    check_initialized();
    try {
        struct sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);

        if (getpeername(m_sockfd, reinterpret_cast<struct sockaddr*>(&addr_storage), &addr_len) != 0) {
            error("getpeername");
        }

        return socket_address(reinterpret_cast<struct sockaddr*>(&addr_storage), addr_len);
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Check if socket is connected
 * @return True if socket is connected
 */
bool socket::is_connected() const
{
    return m_is_connected && m_sockfd != INVALID_SOCKET_VALUE && !m_is_closed;
}

/**
 * @brief Check if socket is closed
 * @return True if socket is closed
 */
bool socket::is_closed() const
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
bool socket::poll(const std::chrono::milliseconds& timeout, int mode)
{
    check_initialized();
    try {
#ifdef _WIN32
        fd_set read_fds, write_fds, except_fds;
        fd_set* p_read_fds = nullptr;
        fd_set* p_write_fds = nullptr;
        fd_set* p_except_fds = nullptr;

        if (mode & SELECT_READ) {
            FD_ZERO(&read_fds);
            FD_SET(m_sockfd, &read_fds);
            p_read_fds = &read_fds;
        }

        if (mode & SELECT_WRITE) {
            FD_ZERO(&write_fds);
            FD_SET(m_sockfd, &write_fds);
            p_write_fds = &write_fds;
        }

        if (mode & SELECT_ERROR) {
            FD_ZERO(&except_fds);
            FD_SET(m_sockfd, &except_fds);
            p_except_fds = &except_fds;
        }

        struct timeval tv;
        tv.tv_sec = static_cast<long>(timeout.count() / 1000);
        tv.tv_usec = static_cast<long>((timeout.count() % 1000) * 1000);

        int result = ::select(static_cast<int>(m_sockfd) + 1, p_read_fds, p_write_fds, p_except_fds, &tv);

        if (result == SOCKET_ERROR_VALUE) {
            error("select");
        }

        return result > 0;
#else
        struct pollfd pfd;
        pfd.fd = m_sockfd;
        pfd.events = 0;
        pfd.revents = 0;

        if (mode & SELECT_READ) {
            pfd.events |= POLLIN;
        }

        if (mode & SELECT_WRITE) {
            pfd.events |= POLLOUT;
        }

        if (mode & SELECT_ERROR) {
            pfd.events |= POLLERR;
        }

        int result = ::poll(&pfd, 1, static_cast<int>(timeout.count()));

        if (result < 0) {
            if (errno == EINTR) {
                return false;
            }
            error("poll");
        }

        return result > 0;
#endif
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Get number of bytes available for reading
 * @return Number of bytes available
 * @throws std::system_error
 */
int socket::available()
{
    check_initialized();
    try {
        int result;

#ifdef _WIN32
        u_long bytes_available = 0;
        if (ioctlsocket(m_sockfd, FIONREAD, &bytes_available) != 0) {
            error("ioctlsocket");
        }
        result = static_cast<int>(bytes_available);
#else
        if (::ioctl(m_sockfd, FIONREAD, &result) != 0) {
            error("ioctl");
        }
#endif

        return result;
    } catch (const std::exception&) {
        throw;
    }
}

/**
 * @brief Initialize socket with specific address family and type
 * @param family Address family
 * @param type socket type
 * @throws std::system_error
 */
void socket::init(socket_address::Family family, Type type)
{
    if (m_sockfd != INVALID_SOCKET_VALUE) {
        throw std::logic_error("socket already initialized");
    }

    try {
        int af = static_cast<int>(family);
        int socket_type = (type == STREAM_SOCKET) ? SOCK_STREAM :
                         (type == DATAGRAM_SOCKET) ? SOCK_DGRAM : SOCK_RAW;

        init_socket(af, socket_type, 0);
        m_is_closed = false;
        m_is_connected = false;
    } catch (const std::exception&) {
        m_sockfd = INVALID_SOCKET_VALUE;
        m_is_closed = true;
        m_is_connected = false;
        throw;
    }
}

/**
 * @brief Initialize socket with address family
 * @param af Address family (AF_INET, AF_INET6)
 * @throws std::system_error
 */
void socket::init(int af)
{
    init_socket(af, SOCK_STREAM, 0);
}

/**
 * @brief Initialize socket with full parameters
 * @param af Address family
 * @param type socket type
 * @param proto Protocol (default 0)
 * @throws std::system_error
 */
void socket::init_socket(int af, int type, int proto)
{
    if (m_sockfd != INVALID_SOCKET_VALUE) {
        throw std::logic_error("socket already initialized");
    }

    m_sockfd = ::socket(af, type, proto);

    if (m_sockfd == INVALID_SOCKET_VALUE) {
        error("socket");
    }
}

/**
 * @brief Check if socket is initialized
 * @throws std::system_error if not initialized
 */
void socket::check_initialized() const
{
    if (m_sockfd == INVALID_SOCKET_VALUE) {
        throw std::logic_error("socket not initialized");
    }
    if (m_is_closed) {
        throw std::logic_error("socket is closed");
    }
}

/**
 * @brief Reset socket descriptor
 * @param fd New socket descriptor (default INVALID_SOCKET_VALUE)
 */
void socket::reset(socket_t fd)
{
    if (m_sockfd != INVALID_SOCKET_VALUE) {
        throw std::logic_error("socket already initialized");
    }

    m_sockfd = fd;
}

/**
 * @brief Perform ioctl operation
 * @param request ioctl request code
 * @param arg Request argument
 */
void socket::ioctl(unsigned long request, int& arg)
{
#ifdef _WIN32
    if (ioctlsocket(m_sockfd, request, reinterpret_cast<u_long*>(&arg)) != 0) {
        error("ioctlsocket");
    }
#else
    if (::ioctl(m_sockfd, request, &arg) != 0) {
        error("ioctl");
    }
#endif
}

/**
 * @brief Perform ioctl operation
 * @param request ioctl request code
 * @param arg Request argument pointer
 */
void socket::ioctl(unsigned long request, void* arg)
{
#ifdef _WIN32
    if (ioctlsocket(m_sockfd, request, static_cast<u_long*>(arg)) != 0) {
        error("ioctlsocket");
    }
#else
    if (::ioctl(m_sockfd, request, arg) != 0) {
        error("ioctl");
    }
#endif
}

/**
 * @brief Check for broken timeout implementation and apply workaround
 * @param mode Select mode
 */
void socket::check_broken_timeout(SelectMode /* mode */)
{
    // Implementation placeholder for broken timeout handling
    // This would be used to work around platform-specific timeout issues
}

/**
 * @brief Get description of last socket error
 * @return Error description string
 */
std::string socket::last_error_desc()
{
    int err = last_error();

#ifdef _WIN32
    char* msg = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(flags, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<char*>(&msg), 0, nullptr);

    std::string result;
    if (msg) {
        result = msg;
        LocalFree(msg);
    }
    else {
        result = "Unknown error";
    }

    return result;
#else
    return std::string(strerror(err));
#endif
}

/**
 * @brief Throw exception based on last error
 * @throws std::system_error
 */
void socket::error()
{
    error(last_error());
}

/**
 * @brief Throw exception with message based on last error
 * @param arg Additional error message
 * @throws std::system_error
 */
void socket::error(const std::string& arg)
{
    error(last_error(), arg);
}

/**
 * @brief Throw exception for specific error code
 * @param code Error code
 * @throws std::system_error
 */
void socket::error(int code)
{
    std::ostringstream oss;
    oss << "socket error " << code;

#ifdef _WIN32
    char* msg = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(flags, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<char*>(&msg), 0, nullptr);

    if (msg) {
        oss << ": " << msg;
        LocalFree(msg);
    }
#else
    oss << ": " << strerror(code);
#endif

    throw std::runtime_error(oss.str());
}

/**
 * @brief Throw exception for specific error code with message
 * @param code Error code
 * @param arg Additional error message
 * @throws std::system_error
 */
void socket::error(int code, const std::string& arg)
{
    std::ostringstream oss;
    oss << "socket error in " << arg << " (" << code << ")";

#ifdef _WIN32
    char* msg = nullptr;
    DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessageA(flags, nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   reinterpret_cast<char*>(&msg), 0, nullptr);

    if (msg) {
        oss << ": " << msg;
        LocalFree(msg);
    }
#else
    oss << ": " << strerror(code);
#endif

    throw std::runtime_error(oss.str());
}

#ifdef _WIN32
/**
 * @brief Initialize Winsock library (Windows only)
 */
void socket::init_winsock()
{
    if (!s_winsock_initialized) {
        WSADATA wsa_data;
        int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
        if (result != 0) {
            throw std::runtime_error("Failed to initialize Winsock");
        }
        s_winsock_initialized = true;
    }
    ++s_winsock_ref_count;
}

/**
 * @brief Cleanup Winsock library (Windows only)
 */
void socket::cleanup_winsock()
{
    if (s_winsock_initialized && --s_winsock_ref_count == 0) {
        WSACleanup();
        s_winsock_initialized = false;
    }
}
#else
void socket::init_winsock()
{
    // No-op on non-Windows platforms
}

void socket::cleanup_winsock()
{
    // No-op on non-Windows platforms
}
#endif

} // namespace fb
