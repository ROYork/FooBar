#include <algorithm>
#include <fb/udp_client.h>
#include <fb/udp_socket.h>
#include <stdexcept>
#include <system_error>

namespace fb {

/**
 * @brief Default constructor creates unconnected UDP client
 */
udp_client::udp_client() :
  m_is_connected(false),
  m_timeout(DEFAULT_TIMEOUT)
{
  init_client(socket_address::Family::IPv4);
}

/**
 * @brief Create UDP client with specific address family
 * @param family Address family (IPv4 or IPv6)
 */
udp_client::udp_client(socket_address::Family family) :
  m_is_connected(false),
  m_timeout(DEFAULT_TIMEOUT)
{
  init_client(family);
}

/**
 * @brief Create UDP client and bind to local address
 * @param local_address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR
 * @throws std::system_error, std::invalid_argument
 */
udp_client::udp_client(const socket_address & local_address, bool reuse_address) :
  m_socket(local_address, reuse_address),
  m_is_connected(false),
  m_timeout(DEFAULT_TIMEOUT)
{
}

/**
 * @brief Create UDP client and connect to remote address
 * @param remote_address Remote address to connect to
 * @param local_address Local address to bind to (optional)
 * @throws std::system_error, std::invalid_argument
 */
udp_client::udp_client(const socket_address & remote_address, const socket_address & local_address) :
  m_is_connected(false),
  m_timeout(DEFAULT_TIMEOUT)
{
  // Initialize socket with appropriate family
  init_client(remote_address.family());

  // Bind to local address if provided and not default
  if (local_address.port() != 0 || !local_address.host().empty())
  {
    m_socket.bind(local_address, true);
  }

  // Connect to remote address
  connect(remote_address);
}

/**
 * @brief Move constructor
 */
udp_client::udp_client(udp_client && other) noexcept :
  m_socket(std::move(other.m_socket)),
  m_is_connected(other.m_is_connected),
  m_remote_address(std::move(other.m_remote_address)),
  m_timeout(other.m_timeout)
{
  other.m_is_connected = false;
}

/**
 * @brief Move assignment operator
 */
udp_client & udp_client::operator=(udp_client && other) noexcept
{
  if (this != &other)
  {
    m_socket             = std::move(other.m_socket);
    m_is_connected       = other.m_is_connected;
    m_remote_address     = std::move(other.m_remote_address);
    m_timeout            = other.m_timeout;

    other.m_is_connected = false;
  }
  return *this;
}

/**
 * @brief Destructor
 */
udp_client::~udp_client() = default;

/**
 * @brief Connect to remote address (sets default destination)
 * @param address Remote address to connect to
 * @throws std::system_error, std::invalid_argument
 * @note This doesn't create a TCP-style connection, just sets default
 * destination
 */
void udp_client::connect(const socket_address & address)
{
  validate_socket();

  m_socket.connect(address);
  m_remote_address = address;
  m_is_connected   = true;

  // Emit signal if listeners are connected
  if (onConnected.slot_count() > 0)
  {
    onConnected.emit(address);
  }
}

/**
 * @brief Bind to local address
 * @param address Local address to bind to
 * @param reuse_address Enable SO_REUSEADDR
 * @throws std::system_error, std::invalid_argument
 */
void udp_client::bind(const socket_address & address, bool reuse_address)
{
  validate_socket();

  m_socket.bind(address, reuse_address);
}

/**
 * @brief Disconnect from remote address
 */
void udp_client::disconnect()
{
  if (m_is_connected)
  {
    m_is_connected   = false;
    m_remote_address = socket_address();

    // Emit signal if listeners are connected
    if (onDisconnected.slot_count() > 0)
    {
      onDisconnected.emit();
    }
  }

  // Note: UDP "disconnect" doesn't close socket, just clears default
  // destination
}

/**
 * @brief Close the UDP client
 */
void udp_client::close()
{
  m_socket.close();
  m_is_connected = false;
}

/**
 * @brief Send data to connected address
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @return Number of bytes actually sent
 * @throws std::system_error if not connected or send fails
 */
int udp_client::send(const void * buffer, std::size_t length)
{
  if (!m_is_connected)
  {
    throw std::logic_error("udp_client is not connected");
  }

  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    int sent = m_socket.send_bytes(buffer, static_cast<int>(length));
    if (sent > 0)
    {
      if (onDataSent.slot_count() > 0)
      {
        onDataSent.emit(buffer, static_cast<std::size_t>(sent));
      }
    }
    return sent;
  }
  catch (const std::exception & ex)
  {
    if (onSendError.slot_count() > 0)
    {
      onSendError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Send string to connected address
 * @param message String to send
 * @return Number of bytes actually sent
 * @throws std::system_error if not connected or send fails
 */
int udp_client::send(const std::string & message)
{
  return send(message.data(), message.size());
}

/**
 * @brief Receive data from connected address
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @return Number of bytes actually received
 * @throws std::system_error if not connected or receive fails
 */
int udp_client::receive(void * buffer, std::size_t length)
{
  if (!m_is_connected)
  {
    throw std::logic_error("udp_client is not connected");
  }

  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    int received = m_socket.receive_bytes(buffer, static_cast<int>(length));
    if (received > 0)
    {
      if (onDataReceived.slot_count() > 0)
      {
        onDataReceived.emit(buffer, static_cast<std::size_t>(received));
      }
    }
    return received;
  }
  catch (const std::exception & ex)
  {
    if (onReceiveError.slot_count() > 0)
    {
      onReceiveError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Receive string from connected address
 * @param message String to store received data
 * @param max_length Maximum number of bytes to receive
 * @return Number of bytes actually received
 * @throws std::system_error if not connected or receive fails
 */
int udp_client::receive(std::string & message, std::size_t max_length)
{
  if (!m_is_connected)
  {
    throw std::logic_error("udp_client is not connected");
  }

  validate_socket();

  message.resize(max_length);
  int received = m_socket.receive_bytes(&message[0], static_cast<int>(max_length));

  if (received >= 0)
  {
    message.resize(static_cast<std::size_t>(received));
  }
  else
  {
    message.clear();
  }

  return received;
}

/**
 * @brief Send data to specific address
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param address Destination address
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_client::send_to(const void * buffer, std::size_t length, const socket_address & address)
{
  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    int sent = m_socket.send_to(buffer, static_cast<int>(length), address);
    if (sent > 0)
    {
      if (onDataSent.slot_count() > 0)
      {
        onDataSent.emit(buffer, static_cast<std::size_t>(sent));
      }
    }
    return sent;
  }
  catch (const std::exception & ex)
  {
    if (onSendError.slot_count() > 0)
    {
      onSendError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Send string to specific address
 * @param message String to send
 * @param address Destination address
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_client::send_to(const std::string & message, const socket_address & address)
{
  return send_to(message.data(), message.size(), address);
}

/**
 * @brief Receive data from any source
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @param sender_address Output parameter for sender address
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_client::receive_from(void * buffer, std::size_t length, socket_address & sender_address)
{
  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    int received = m_socket.receive_from(buffer, static_cast<int>(length), sender_address);
    if (received > 0)
    {
      if (onDatagramReceived.slot_count() > 0)
      {
        onDatagramReceived.emit(buffer, static_cast<std::size_t>(received), sender_address);
      }
    }
    return received;
  }
  catch (const std::exception & ex)
  {
    if (onReceiveError.slot_count() > 0)
    {
      onReceiveError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Receive string from any source
 * @param message String to store received data
 * @param max_length Maximum number of bytes to receive
 * @param sender_address Output parameter for sender address
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_client::receive_from(std::string & message,
                             std::size_t max_length,
                             socket_address & sender_address)
{
  validate_socket();

  message.resize(max_length);
  int received = m_socket.receive_from(&message[0], static_cast<int>(max_length), sender_address);

  if (received >= 0)
  {
    message.resize(static_cast<std::size_t>(received));
  }
  else
  {
    message.clear();
  }

  return received;
}

/**
 * @brief Send data with timeout
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param timeout Send timeout
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_client::send_with_timeout(const void * buffer,
                                  std::size_t length,
                                  const std::chrono::milliseconds & timeout)
{
  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    // Check if socket is ready for writing
    if (!m_socket.poll_write(timeout))
    {
      throw std::system_error(std::make_error_code(std::errc::timed_out), "Send timeout");
    }

    return send(buffer, length);
  }
  catch (const std::system_error & ex)
  {
    if (ex.code() != std::make_error_code(std::errc::timed_out))
    {
      throw;
    }
    if (onTimeoutError.slot_count() > 0)
    {
      onTimeoutError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Receive data with timeout
 * @param buffer Pointer to receive buffer
 * @param length Maximum number of bytes to receive
 * @param timeout Receive timeout
 * @return Number of bytes actually received
 * @throws std::system_error
 */
int udp_client::receive_with_timeout(void * buffer,
                                     std::size_t length,
                                     const std::chrono::milliseconds & timeout)
{
  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    // Check if data is available for reading
    if (!m_socket.poll_read(timeout))
    {
      throw std::system_error(std::make_error_code(std::errc::timed_out), "Receive timeout");
    }

    return receive(buffer, length);
  }
  catch (const std::system_error & ex)
  {
    if (ex.code() != std::make_error_code(std::errc::timed_out))
    {
      throw;
    }
    if (onTimeoutError.slot_count() > 0)
    {
      onTimeoutError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Enable/disable broadcast capability
 * @param flag Enable/disable broadcast
 * @throws std::system_error
 */
void udp_client::set_broadcast(bool flag)
{
  validate_socket();
  m_socket.set_broadcast(flag);
}

/**
 * @brief Get broadcast setting
 * @return True if broadcast is enabled
 * @throws std::system_error
 */
bool udp_client::get_broadcast()
{
  validate_socket();
  return m_socket.get_broadcast();
}

/**
 * @brief Send broadcast packet
 * @param buffer Pointer to data buffer
 * @param length Number of bytes to send
 * @param port Port number for broadcast
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_client::broadcast(const void * buffer, std::size_t length, std::uint16_t port)
{
  validate_socket();
  validate_buffer(buffer, length);

  try
  {
    // Enable broadcast if not already enabled
    if (!get_broadcast())
    {
      set_broadcast(true);
    }

    // Create broadcast address
    socket_address broadcast_addr;
    if (m_socket.address().family() == socket_address::Family::IPv4)
    {
      broadcast_addr = socket_address("255.255.255.255", port);
    }
    else
    {
      // IPv6 doesn't have broadcast, use multicast instead
      broadcast_addr = socket_address("ff02::1", port); // All nodes multicast
    }

    int sent = send_to(buffer, length, broadcast_addr);
    if (sent > 0)
    {
      if (onBroadcastSent.slot_count() > 0)
      {
        onBroadcastSent.emit(buffer, static_cast<std::size_t>(sent));
      }
    }
    return sent;
  }
  catch (const std::exception & ex)
  {
    if (onSendError.slot_count() > 0)
    {
      onSendError.emit(ex.what());
    }
    throw;
  }
}

/**
 * @brief Send broadcast string
 * @param message String to broadcast
 * @param port Port number for broadcast
 * @return Number of bytes actually sent
 * @throws std::system_error
 */
int udp_client::broadcast(const std::string & message, std::uint16_t port)
{
  return broadcast(message.data(), message.size(), port);
}

/**
 * @brief Set socket timeout for send/receive operations
 * @param timeout Timeout duration
 * @throws std::system_error
 */
void udp_client::set_timeout(const std::chrono::milliseconds & timeout)
{
  validate_socket();

  m_timeout = timeout;
  m_socket.set_send_timeout(timeout);
  m_socket.set_receive_timeout(timeout);
}

/**
 * @brief Get socket timeout
 * @return Current timeout setting
 * @throws std::system_error
 */
std::chrono::milliseconds udp_client::get_timeout()
{
  validate_socket();
  return m_timeout;
}

/**
 * @brief Set receive buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void udp_client::set_receive_buffer_size(int size)
{
  validate_socket();
  m_socket.set_receive_buffer_size(size);
}

/**
 * @brief Get receive buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int udp_client::get_receive_buffer_size()
{
  validate_socket();
  return m_socket.get_receive_buffer_size();
}

/**
 * @brief Set send buffer size
 * @param size Buffer size in bytes
 * @throws std::system_error
 */
void udp_client::set_send_buffer_size(int size)
{
  validate_socket();
  m_socket.set_send_buffer_size(size);
}

/**
 * @brief Get send buffer size
 * @return Buffer size in bytes
 * @throws std::system_error
 */
int udp_client::get_send_buffer_size()
{
  validate_socket();
  return m_socket.get_send_buffer_size();
}

/**
 * @brief Check if client is connected to a specific address
 * @return True if connected
 */
bool udp_client::is_connected() const
{
  return m_is_connected && !m_socket.is_closed();
}

/**
 * @brief Check if client is closed
 * @return True if closed
 */
bool udp_client::is_closed() const
{
  return m_socket.is_closed();
}

/**
 * @brief Get local address
 * @return Local socket address
 * @throws std::system_error
 */
socket_address udp_client::local_address() const
{
  validate_socket();
  return m_socket.address();
}

/**
 * @brief Get remote address (if connected)
 * @return Remote socket address
 * @throws std::system_error if not connected
 */
socket_address udp_client::remote_address() const
{
  if (!m_is_connected)
  {
    throw std::logic_error("udp_client is not connected");
  }
  return m_remote_address;
}

/**
 * @brief Check if data is available to read
 * @param timeout Poll timeout
 * @return True if data is available
 * @throws std::system_error
 */
bool udp_client::has_data_available(const std::chrono::milliseconds & timeout)
{
  validate_socket();
  return m_socket.poll_read(timeout);
}

/**
 * @brief Get the underlying udp_socket
 * @return Reference to udp_socket
 * @throws std::system_error if socket is invalid
 */
udp_socket & udp_client::socket()
{
  validate_socket();
  return m_socket;
}

/**
 * @brief Get the underlying udp_socket (const version)
 * @return Const reference to udp_socket
 * @throws std::system_error if socket is invalid
 */
const udp_socket & udp_client::socket() const
{
  validate_socket();
  return m_socket;
}

/**
 * @brief Validate that socket is in a usable state
 * @throws std::system_error if socket is invalid
 */
void udp_client::validate_socket() const
{
  if (m_socket.is_closed())
  {
    throw std::logic_error("udp_client socket is closed");
  }
}

/**
 * @brief Validate buffer parameters
 * @param buffer Buffer pointer
 * @param length Buffer length
 * @throws std::system_error if parameters are invalid
 */
void udp_client::validate_buffer(const void * buffer, std::size_t length) const
{
  if (!buffer)
  {
    throw std::invalid_argument("Buffer pointer is null");
  }

  if (length == 0)
  {
    throw std::invalid_argument("Buffer length is zero");
  }

  if (length > DEFAULT_BUFFER_SIZE)
  {
    throw std::length_error("Buffer length exceeds maximum UDP payload size");
  }
}

/**
 * @brief Initialize client with address family
 * @param family Address family
 */
void udp_client::init_client(socket_address::Family family)
{
  m_socket = udp_socket(family);
  m_socket.set_send_timeout(m_timeout);
  m_socket.set_receive_timeout(m_timeout);
}

} // namespace fb
