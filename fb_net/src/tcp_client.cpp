#include <fb/tcp_client.h>
#include <algorithm>
#include <cstring>
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

namespace fb
{

/**
 * @brief  Default constructor creates an uninitialized TCP socket
 */
tcp_client::tcp_client() :
  socket_base()
{
  init_tcp_socket(socket_address::IPv4);
}

/**
 * @brief Create a TCP socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
tcp_client::tcp_client(socket_address::Family family) :
  socket_base()
{
  init_tcp_socket(family);
}

/**
 * @brief Create a TCP socket and connect to remote address
 * @param address Remote address to connect to
 * @throws std::system_error if the connection attempt fails
 */
tcp_client::tcp_client(const socket_address &address) :
  socket_base()
{
  init_tcp_socket(address.family());
  connect(address);
}

/**
 * @brief Create a TCP socket and connect to remote address with timeout
 * @param address Remote address to connect to
 * @param timeout Connection timeout
 * @throws std::system_error if the connection attempt fails or times out
 */
tcp_client::tcp_client(const socket_address &address,
                       const std::chrono::milliseconds &timeout) :
  socket_base()
{
  init_tcp_socket(address.family());
  connect(address, timeout);
}

/**
 * @brief Create tcp_client from existing socket descriptor (for accept
 * operations)
 * @param sockfd Socket descriptor to wrap
 */
tcp_client::tcp_client(socket_t sockfd) :
  socket_base(sockfd)
{
}

/**
 * @brief Move constructor
 */
tcp_client::tcp_client(tcp_client &&other) noexcept :
  socket_base(std::move(other))
{
}

/**
 * @brief Move assignment operator
 */
tcp_client &tcp_client::operator=(tcp_client &&other) noexcept
{
  socket_base::operator=(std::move(other));
  return *this;
}

/**
 * @brief Connect to remote address
 * @param address Remote address to connect to
 * @throws std::system_error if the connection attempt fails
 */
void tcp_client::connect(const socket_address &address)
{
  try
  {
    socket_base::connect(address);
    // Emit connected signal if listeners exist
    if (onConnected.slot_count() > 0)
    {
      onConnected.emit(address);
    }
  }
  catch (const std::exception &ex)
  {
    // Emit error signal if listeners exist
    if (onConnectionError.slot_count() > 0)
    {
      onConnectionError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Connect to remote address with timeout
 * @param address Remote address to connect to
 * @param timeout Connection timeout
 * @throws std::system_error if the connection attempt fails or times out
 */
void tcp_client::connect(const socket_address &address,
                         const std::chrono::milliseconds &timeout)
{
  try
  {
    socket_base::connect(address, timeout);
    // Emit connected signal if listeners exist
    if (onConnected.slot_count() > 0)
    {
      onConnected.emit(address);
    }
  }
  catch (const std::exception &ex)
  {
    // Emit error signal if listeners exist
    if (onConnectionError.slot_count() > 0)
    {
      onConnectionError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Connect using non-blocking mode
 * @param address Remote address to connect to
 * @throws std::system_error if the connection cannot be initiated
 */
void tcp_client::connect_non_blocking(const socket_address &address)
{
  check_initialized();

  // Set socket to non-blocking mode
  set_blocking(false);

  try
  {
    // Call socket_base::connect directly to avoid emitting false error signals
    // for expected EINPROGRESS/EWOULDBLOCK conditions
    socket_base::connect(address);
    // If we get here, connection completed immediately
    if (onConnected.slot_count() > 0)
    {
      onConnected.emit(address);
    }
  }
  catch (const std::system_error &ex)
  {
    const auto code = ex.code();
    if (code == std::make_error_code(std::errc::operation_in_progress) ||
        code == std::make_error_code(std::errc::operation_would_block))
    {
      // Expected for non-blocking connect - connection still pending.
      // Do NOT emit onConnectionError as this is normal behavior.
      return;
    }
    // Real error - emit signal and rethrow
    if (onConnectionError.slot_count() > 0)
    {
      onConnectionError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Send data over the connection
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param flags Send flags (default 0)
 * @return Number of bytes actually sent
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on send failure
 */
int tcp_client::send_bytes(const void *buffer, int length, int flags)
{
  check_initialized();
  if (!buffer)
  {
    throw std::invalid_argument("Buffer cannot be null");
  }
  if (length < 0)
  {
    throw std::invalid_argument("Length cannot be negative");
  }
  if (length == 0)
  {
    return 0;
  }

  int sent = ::send(sockfd(), static_cast<const char *>(buffer), length, flags);
  if (sent < 0)
  {
    // Emit error signal if listeners exist
    if (onSendError.slot_count() > 0)
    {
      onSendError.emit("Failed to send data");
    }
    error("Failed to send data");
  }
  else if (sent > 0)
  {
    // Emit data sent signal if listeners exist
    if (onDataSent.slot_count() > 0)
    {
      onDataSent.emit(static_cast<size_t>(sent));
    }
  }
  return sent;
}

/**
 * @brief Receive data from the connection
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @param flags Receive flags (default 0)
 * @return Number of bytes actually received, 0 if connection closed
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on receive failure
 */
int tcp_client::receive_bytes(void *buffer, int length, int flags)
{
  check_initialized();
  if (!buffer)
  {
    throw std::invalid_argument("Buffer cannot be null");
  }
  if (length < 0)
  {
    throw std::invalid_argument("Length cannot be negative");
  }
  if (length == 0)
  {
    return 0;
  }

  int received = ::recv(sockfd(), static_cast<char *>(buffer), length, flags);
  if (received < 0)
  {
    // Emit error signal if listeners exist
    if (onReceiveError.slot_count() > 0)
    {
      onReceiveError.emit("Failed to receive data");
    }
    error("Failed to receive data");
  }
  else if (received == 0)
  {
    // Connection closed by peer - update state and emit disconnected signal
    set_connected(false);
    if (onDisconnected.slot_count() > 0)
    {
      onDisconnected.emit();
    }
  }
  else if (received > 0)
  {
    // Emit data received signal if listeners exist
    if (onDataReceived.slot_count() > 0)
    {
      onDataReceived.emit(buffer, static_cast<size_t>(received));
    }
  }
  return received;
}

/**
 * @brief Send string data over the connection
 * @param message String to send
 * @return Number of bytes actually sent
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on receive failure
 */
int tcp_client::send(const std::string &message)
{
  if (message.empty())
  {
    return 0;
  }
  return send_bytes(message.data(), static_cast<int>(message.size()));
}

/**
 * @brief Receive string data from the connection
 * @param buffer Buffer to store received data
 * @param length Maximum number of bytes to receive
 * @return Number of bytes actually received, 0 if connection closed
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on send failure
 */
int tcp_client::receive(std::string &buffer, int length)
{
  if (length <= 0)
  {
    return 0;
  }

  buffer.resize(length);
  int received = receive_bytes(&buffer[0], length);
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

/**
 * @brief Receive exactly the specified number of bytes
 * @param buffer Pointer to receive buffer
 * @param length Number of bytes to receive
 * @param flags Receive flags (default 0)
 * @return Number of bytes received (should equal length unless connection
 * closed)
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on receive failure
 */
int tcp_client::receive_bytes_exact(void *buffer, int length, int flags)
{
  check_initialized();
  if (!buffer)
  {
    throw std::invalid_argument("Buffer cannot be null");
  }
  if (length < 0)
  {
    throw std::invalid_argument("Length cannot be negative");
  }
  if (length == 0)
  {
    return 0;
  }

  char *char_buffer  = static_cast<char *>(buffer);
  int total_received = 0;

  while (total_received < length)
  {
    int received = receive_bytes(char_buffer + total_received,
                                 length - total_received, flags);
    if (received <= 0)
    {
      // Connection closed or error
      break;
    }
    total_received += received;
  }

  return total_received;
}

/**
 * @brief Send all data in buffer
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param flags Send flags (default 0)
 * @return Number of bytes sent (should equal length)
 * @throws std::invalid_argument if the buffer parameters are invalid
 * @throws std::system_error on send failure
 */
int tcp_client::send_bytes_all(const void *buffer, int length, int flags)
{
  check_initialized();
  if (!buffer)
  {
    throw std::invalid_argument("Buffer cannot be null");
  }
  if (length < 0)
  {
    throw std::invalid_argument("Length cannot be negative");
  }
  if (length == 0)
  {
    return 0;
  }

  const char *char_buffer = static_cast<const char *>(buffer);
  int total_sent          = 0;

  while (total_sent < length)
  {
    int sent = send_bytes(char_buffer + total_sent, length - total_sent, flags);
    if (sent <= 0)
    {
      // Connection error or would block
      break;
    }
    total_sent += sent;
  }

  return total_sent;
}

/**
 * @brief Gracefully shutdown the connection
 * @throws std::logic_error if the socket state is invalid
 * @throws std::system_error on shutdown failure
 */
void tcp_client::shutdown()
{
  check_initialized();

  // Emit shutdown signal if listeners exist
  if (onShutdownInitiated.slot_count() > 0)
  {
    onShutdownInitiated.emit();
  }

#ifdef _WIN32
  int rc = ::shutdown(sockfd(), SD_BOTH);
#else
  int rc = ::shutdown(sockfd(), SHUT_RDWR);
#endif

  if (rc != 0)
  {
    error("Failed to shutdown socket");
  }
}

/**
 * @brief Shutdown receive operations
 * @throws std::logic_error if the socket state is invalid
 * @throws std::system_error on shutdown failure
 */
void tcp_client::shutdown_receive()
{
  check_initialized();

#ifdef _WIN32
  int rc = ::shutdown(sockfd(), SD_RECEIVE);
#else
  int rc = ::shutdown(sockfd(), SHUT_RD);
#endif

  if (rc != 0)
  {
    error("Failed to shutdown receive");
  }
}

/**
 * @brief Shutdown send operations
 * @return Status code
 * @throws std::logic_error if the socket state is invalid
 * @throws std::system_error on shutdown failure
 */
int tcp_client::shutdown_send()
{
  check_initialized();

#ifdef _WIN32
  return ::shutdown(sockfd(), SD_SEND);
#else
  return ::shutdown(sockfd(), SHUT_WR);
#endif
}

/**
 * @brief Send urgent data (out-of-band)
 * @param data Urgent data byte
 * @throws std::logic_error if the socket state is invalid
 * @throws std::system_error on send failure
 */
void tcp_client::send_urgent(unsigned char data)
{
  check_initialized();

#ifdef _WIN32
  int rc = ::send(sockfd(), reinterpret_cast<const char *>(&data), sizeof(data),
                  MSG_OOB);
#else
  int rc = ::send(sockfd(), &data, sizeof(data), MSG_OOB);
#endif

  if (rc < 0)
  {
    error("Failed to send urgent data");
  }
}

/**
 * @brief Set TCP no delay option (disable Nagle algorithm)
 * @param flag Enable/disable no delay
 * @throws std::system_error on option failure
 */
void tcp_client::set_no_delay(bool flag) { socket_base::set_no_delay(flag); }

/**
 * @brief Get TCP no delay option
 * @return True if no delay is enabled
 * @throws std::system_error on option failure
 */
bool tcp_client::get_no_delay() { return socket_base::get_no_delay(); }

/**
 * @brief Set keep alive option
 * @param flag Enable/disable keep alive
 * @throws std::system_error on option failure
 */
void tcp_client::set_keep_alive(bool flag)
{
  socket_base::set_keep_alive(flag);
}

/**
 * @brief Get keep alive option
 * @return True if keep alive is enabled
 * @throws std::system_error on option failure
 */
bool tcp_client::get_keep_alive() { return socket_base::get_keep_alive(); }

/**
 * @brief Check if there's data available to read
 * @param timeout Poll timeout
 * @return True if data is available
 * @throws std::system_error on polling failure
 */
bool tcp_client::poll_read(const std::chrono::milliseconds &timeout)
{
  return poll(timeout, socket_base::SELECT_READ);
}

/**
 * @brief Check if socket is ready for writing
 * @param timeout Poll timeout
 * @return True if socket is ready for writing
 * @throws std::system_error on polling failure
 */
bool tcp_client::poll_write(const std::chrono::milliseconds &timeout)
{
  return poll(timeout, socket_base::SELECT_WRITE);
}

/**
 * @brief Initialize TCP socket with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
void tcp_client::init_tcp_socket(socket_address::Family family)
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
 * @brief Handle partial send operations
 * @param buffer Data buffer
 * @param length Data length
 * @param sent Bytes already sent
 * @param flags Send flags
 * @return Total bytes sent
 */
int tcp_client::handle_partial_send(const void *buffer,
                                    int length,
                                    int sent,
                                    int flags)
{
  const char *char_buffer = static_cast<const char *>(buffer);
  int total_sent          = sent;

  while (total_sent < length)
  {
    int bytes_sent =
        send_bytes(char_buffer + total_sent, length - total_sent, flags);
    if (bytes_sent <= 0)
    {
      break;
    }
    total_sent += bytes_sent;
  }

  return total_sent;
}

/**
 * @brief Handle partial receive operations
 * @param buffer Receive buffer
 * @param length Buffer length
 * @param received Bytes already received
 * @param flags Receive flags
 * @return Total bytes received
 */
int tcp_client::handle_partial_receive(void *buffer,
                                       int length,
                                       int received,
                                       int flags)
{
  char *char_buffer  = static_cast<char *>(buffer);
  int total_received = received;

  while (total_received < length)
  {
    int bytes_received = receive_bytes(char_buffer + total_received,
                                       length - total_received, flags);
    if (bytes_received <= 0)
    {
      break;
    }
    total_received += bytes_received;
  }

  return total_received;
}

} // namespace fb
