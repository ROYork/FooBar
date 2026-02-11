#include <fb/tcp_server_connection.h>
#include <chrono>
#include <stdexcept>

namespace fb
{

/**
 * @class fb::tcp_server_connection
 * @brief Base wrapper for a single accepted TCP client connection.
 *
 * The tcp_server hands newly-accepted sockets to concrete subclasses of
 * tcp_server_connection. The base class manages lifetime, exposes common
 * utilities, and delegates protocol logic to the overriding `run()` method.
 */

/**
 * @brief Wrap an accepted client socket.
 *
 * @param socket Connected client socket.
 * @param client_address Remote endpoint associated with the socket.
 * @throws std::system_error If the supplied socket is already closed.
 */
tcp_server_connection::tcp_server_connection(
    tcp_client socket,
    const socket_address &client_address) :
  m_socket(std::move(socket)),
  m_client_address(client_address),
  m_stop_requested(false),
  m_start_time(std::chrono::steady_clock::now())
{
  // Validate that we have a valid socket
  if (m_socket.is_closed())
  {
    throw std::invalid_argument(
        "Invalid socket provided to tcp_server_connection");
  }
}

/**
 * @brief Move construct a connection wrapper.
 *
 * Transfers ownership of the socket and metadata. The moved-from instance is
 * marked as stopped to prevent reuse.
 */
tcp_server_connection::tcp_server_connection(
    tcp_server_connection &&other) noexcept :
  m_socket(std::move(other.m_socket)),
  m_client_address(std::move(other.m_client_address)),
  m_stop_requested(other.m_stop_requested.load()),
  m_start_time(other.m_start_time)
{
  // Reset other's state
  other.m_stop_requested = true;
}

/**
 * @brief Move-assign from another connection wrapper.
 *
 * Any existing socket is replaced and the source is marked as stopped once the
 * transfer completes.
 *
 * @return Reference to this instance.
 */
tcp_server_connection &
tcp_server_connection::operator=(tcp_server_connection &&other) noexcept
{
  if (this != &other)
  {
    m_socket         = std::move(other.m_socket);
    m_client_address = std::move(other.m_client_address);
    m_stop_requested = other.m_stop_requested.load();
    m_start_time     = other.m_start_time;

    // Reset other's state
    other.m_stop_requested = true;
  }
  return *this;
}

/**
 * @brief Drive the connection lifecycle.
 *
 * Invokes the derived `run()` implementation, captures exceptions via
 * `handle_exception()`, and ensures cleanup hooks execute with the socket
 * closed on exit.
 */
void tcp_server_connection::start()
{
  // Emit connection started signal
  if (onConnectionStarted.slot_count() > 0)
  {
    onConnectionStarted.emit();
  }

  try
  {
    validate_socket();
    run();
  }
  catch (const std::exception &ex)
  {
    // Emit exception signal
    if (onException.slot_count() > 0)
    {
      onException.emit(ex);
    }
    handle_exception(ex);
  }

  // Ensure connection cleanup
  try
  {
    // Emit connection closing signal
    if (onConnectionClosing.slot_count() > 0)
    {
      onConnectionClosing.emit();
    }

    on_connection_closing();

    if (is_connected())
    {
      m_socket.close();
    }

    // Emit connection closed signal
    if (onConnectionClosed.slot_count() > 0)
    {
      onConnectionClosed.emit();
    }
  }
  catch (...)
  {
    // Ignore exceptions during cleanup
  }
}

/**
 * @brief Request that the connection terminate.
 *
 * Sets an atomic flag for cooperative `run()` implementations to observe.
 */
void tcp_server_connection::stop() { m_stop_requested = true; }

/**
 * @brief Access the underlying client socket.
 *
 * @return Mutable reference usable for I/O operations.
 * @throws std::system_error If the socket has already been closed.
 */
tcp_client &tcp_server_connection::socket()
{
  validate_socket();
  return m_socket;
}

/**
 * @brief Const access to the client socket.
 *
 * @return Const reference to the socket.
 * @throws std::system_error If the socket has already been closed.
 */
const tcp_client &tcp_server_connection::socket() const
{
  validate_socket();
  return m_socket;
}

/**
 * @brief Get the peer endpoint information.
 *
 * @return Address of the connected client.
 */
const socket_address &tcp_server_connection::client_address() const
{
  return m_client_address;
}

/**
 * @brief Get the local endpoint information.
 *
 * @return Address bound to the server side of the socket.
 * @throws std::system_error If the socket has already been closed.
 */
socket_address tcp_server_connection::local_address() const
{
  validate_socket();
  return m_socket.address();
}

/**
 * @brief Report whether the connection is still active.
 *
 * @return True if the socket remains open and stop has not been requested.
 */
bool tcp_server_connection::is_connected() const
{
  try
  {
    return !m_socket.is_closed() && !m_stop_requested.load();
  }
  catch (...)
  {
    return false;
  }
}

/**
 * @brief Determine whether a stop has been signalled.
 *
 * @return True if `stop()` has been called.
 */
bool tcp_server_connection::stop_requested() const
{
  return m_stop_requested.load();
}

/**
 * @brief Measure how long the connection has been alive.
 *
 * @return Duration since construction.
 */
std::chrono::steady_clock::duration tcp_server_connection::uptime() const
{
  return std::chrono::steady_clock::now() - m_start_time;
}

/**
 * @brief Configure symmetric send/receive timeouts.
 *
 * @param timeout Duration applied to both directions.
 */
void tcp_server_connection::set_timeout(
    const std::chrono::milliseconds &timeout)
{
  validate_socket();
  m_socket.set_send_timeout(timeout);
  m_socket.set_receive_timeout(timeout);
}

/**
 * @brief Toggle the TCP_NODELAY option.
 *
 * @param flag True to disable Nagle's algorithm for lower latency.
 */
void tcp_server_connection::set_no_delay(bool flag)
{
  validate_socket();
  m_socket.set_no_delay(flag);
}

/**
 * @brief Enable or disable TCP keepalive probes.
 *
 * @param flag True to enable keepalive packets.
 */
void tcp_server_connection::set_keep_alive(bool flag)
{
  validate_socket();
  m_socket.set_keep_alive(flag);
}

/**
 * @brief Default exception handler for connection logic.
 *
 * @param ex Exception thrown during `run()`.
 *
 * The base implementation marks the connection as stopped; subclasses may
 * override to log or translate errors.
 */
void tcp_server_connection::handle_exception(const std::exception &ex) noexcept
{
  // Default implementation: could log the exception in a real implementation
  // For now, we just ensure the connection stops
  static_cast<void>(ex); // Avoid unused parameter warning
  m_stop_requested = true;

  // In a production system, you might want to log this:
  // LOG_ERROR("TCPServerConnection exception: " << ex.what());
}

/**
 * @brief Hook invoked just before the connection closes.
 *
 * Override to perform additional cleanup. The base implementation does nothing.
 */
void tcp_server_connection::on_connection_closing() noexcept
{
  // Default implementation does nothing
  // Derived classes can override this for cleanup
}

/**
 * @brief Ensure the socket handle remains valid.
 *
 * @throws std::system_error If the connection has already been closed.
 */
void tcp_server_connection::validate_socket() const
{
  if (m_socket.is_closed())
  {
    throw std::logic_error("TCPServerConnection socket is closed");
  }
}

} // namespace fb
