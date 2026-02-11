#include <fb/socket_stream.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace fb
{

// socket_stream_buf Implementation
// ================================

/**
 * @brief Constructor with socket reference
 * @param socket Reference to the underlying tcp_client
 * @param buffer_size Size of internal read/write buffers (default 8192)
 */
socket_stream_buf::socket_stream_buf(tcp_client &socket,
                                     std::size_t buffer_size) :
  m_socket(socket),
  m_buffer_size(buffer_size),
  m_input_buffer_initialized(false),
  m_output_buffer_initialized(false)
{
  if (m_buffer_size == 0)
  {
    throw std::invalid_argument("Buffer size cannot be zero");
  }
}

/**
 * @brief Destructor - flushes any pending data
 */
socket_stream_buf::~socket_stream_buf()
{
  try
  {
    sync(); // Flush any pending output
  }
  catch (...)
  {
    // Destructor should not throw
  }
}

/**
 * @brief Get the underlying socket
 * @return Reference to the tcp_client
 */
tcp_client &socket_stream_buf::socket() { return m_socket; }

/**
 * @brief Get buffer size
 * @return Current buffer size
 */
std::size_t socket_stream_buf::buffer_size() const { return m_buffer_size; }

/**
 * @brief Set buffer size (only when buffers are empty)
 * @param size New buffer size
 * @throws std::invalid_argument if size is zero
 * @throws std::logic_error if buffers still contain data
 */
void socket_stream_buf::set_buffer_size(std::size_t size)
{
  if (size == 0)
  {
    throw std::invalid_argument("Buffer size cannot be zero");
  }

  // Can only change buffer size when buffers are empty
  if ((m_input_buffer_initialized && gptr() != egptr()) ||
      (m_output_buffer_initialized && pptr() != pbase()))
  {
    throw std::logic_error(
        "Cannot change buffer size when buffers contain data");
  }

  m_buffer_size = size;

  // Reset buffers if they were initialized
  if (m_input_buffer_initialized)
  {
    m_input_buffer_initialized = false;
    setg(nullptr, nullptr, nullptr);
  }
  if (m_output_buffer_initialized)
  {
    m_output_buffer_initialized = false;
    setp(nullptr, nullptr);
  }
}

/**
 * @brief Initialize input buffer
 */
void socket_stream_buf::init_input_buffer()
{
  if (!m_input_buffer_initialized)
  {
    m_input_buffer = std::make_unique<char[]>(m_buffer_size);
    setg(m_input_buffer.get(), m_input_buffer.get(), m_input_buffer.get());
    m_input_buffer_initialized = true;
  }
}

/**
 * @brief Initialize output buffer
 */
void socket_stream_buf::init_output_buffer()
{
  if (!m_output_buffer_initialized)
  {
    m_output_buffer = std::make_unique<char[]>(m_buffer_size);
    setp(m_output_buffer.get(), m_output_buffer.get() + m_buffer_size);
    m_output_buffer_initialized = true;
  }
}

std::streambuf::int_type socket_stream_buf::underflow()
{
  // If we have data in buffer, return it
  if (gptr() < egptr())
  {
    return traits_type::to_int_type(*gptr());
  }

  init_input_buffer();

  // Fill buffer from socket
  int bytes_read = fill_input_buffer();
  if (bytes_read <= 0)
  {
    return traits_type::eof();
  }

  return traits_type::to_int_type(*gptr());
}

std::streambuf::int_type socket_stream_buf::uflow()
{
  int_type result = underflow();
  if (result != traits_type::eof())
  {
    gbump(1); // Advance get pointer
  }
  return result;
}

std::streamsize socket_stream_buf::showmanyc()
{
  try
  {
    int available = m_socket.available();
    return std::max(0, available);
  }
  catch (...)
  {
    return 0;
  }
}

std::streambuf::int_type socket_stream_buf::overflow(int_type c)
{
  init_output_buffer();

  // Flush current buffer content
  if (flush_output_buffer() < 0)
  {
    return traits_type::eof();
  }

  // If we have a character to put, put it in the now-empty buffer
  if (c != traits_type::eof())
  {
    *pptr() = traits_type::to_char_type(c);
    pbump(1);
  }

  return traits_type::not_eof(c);
}

int socket_stream_buf::sync()
{
  if (m_output_buffer_initialized && pptr() > pbase())
  {
    return flush_output_buffer() >= 0 ? 0 : -1;
  }
  return 0;
}

std::streambuf *socket_stream_buf::setbuf(char *s, std::streamsize n)
{
  // Standard setbuf behavior - we ignore external buffers
  // and use our own managed buffers
  static_cast<void>(s);
  static_cast<void>(n);
  return this;
}

/**
 * @brief Flush output buffer to socket
 * @return Number of bytes written, -1 on error
 */
int socket_stream_buf::flush_output_buffer()
{
  if (!m_output_buffer_initialized || pptr() == pbase())
  {
    return 0; // Nothing to flush
  }

  try
  {
    std::streamsize bytes_to_send = pptr() - pbase();
    int bytes_sent =
        m_socket.send_bytes_all(pbase(), static_cast<int>(bytes_to_send));

    if (bytes_sent == bytes_to_send)
    {
      // Reset output buffer pointers
      setp(m_output_buffer.get(), m_output_buffer.get() + m_buffer_size);
      return static_cast<int>(bytes_sent);
    }
    else
    {
      return -1; // Partial send or error
    }
  }
  catch (...)
  {
    return -1;
  }
}

/**
 * @brief Fill input buffer from socket
 * @return Number of bytes read, 0 on EOF, -1 on error
 */
int socket_stream_buf::fill_input_buffer()
{
  if (!m_input_buffer_initialized)
  {
    return -1;
  }

  try
  {
    int bytes_read = m_socket.receive_bytes(m_input_buffer.get(),
                                            static_cast<int>(m_buffer_size));
    if (bytes_read > 0)
    {
      setg(m_input_buffer.get(), m_input_buffer.get(),
           m_input_buffer.get() + bytes_read);
    }
    return bytes_read;
  }
  catch (...)
  {
    return -1;
  }
}

// socket_stream Implementation
// ===========================

/**
 * @brief Constructor with existing tcp_client
 * @param socket tcp_client to wrap (must be connected)
 * @param buffer_size Size of internal I/O buffers (default 8192)
 */
socket_stream::socket_stream(tcp_client &socket, std::size_t buffer_size) :
  std::iostream(nullptr),
  m_socket(socket),
  m_stream_buf(std::make_unique<socket_stream_buf>(socket, buffer_size))
{
  std::ios::rdbuf(m_stream_buf.get());
}

/**
 * @brief Constructor with socket address - creates and connects tcp_client
 * @param address Remote address to connect to
 * @param buffer_size Size of internal I/O buffers (default 8192)
 * @throws std::system_error if the connection attempt fails
 */
socket_stream::socket_stream(const socket_address &address,
                             std::size_t buffer_size) :
  std::iostream(nullptr),
  m_owned_socket(std::make_unique<tcp_client>(address)),
  m_socket(*m_owned_socket),
  m_stream_buf(std::make_unique<socket_stream_buf>(m_socket, buffer_size))
{
  std::ios::rdbuf(m_stream_buf.get());
}

/**
 * @brief Constructor with socket address and timeout
 * @param address Remote address to connect to
 * @param timeout Connection timeout
 * @param buffer_size Size of internal I/O buffers (default 8192)
 * @throws std::system_error if the connection attempt fails or times out
 */
socket_stream::socket_stream(const socket_address &address,
                             const std::chrono::milliseconds &timeout,
                             std::size_t buffer_size) :
  std::iostream(nullptr),
  m_owned_socket(std::make_unique<tcp_client>(address, timeout)),
  m_socket(*m_owned_socket),
  m_stream_buf(std::make_unique<socket_stream_buf>(m_socket, buffer_size))
{
  std::ios::rdbuf(m_stream_buf.get());
}

/**
 * @brief Destructor - automatically closes connection if needed
 */
socket_stream::~socket_stream()
{
  try
  {
    // Flush any pending data
    if (m_stream_buf)
    {
      m_stream_buf->pubsync();
    }
  }
  catch (...)
  {
    // Destructor should not throw
  }
}

/**
 * @brief Get the underlying socket
 * @return Reference to the tcp_client
 */
tcp_client &socket_stream::socket() { return m_socket; }

/**
 * @brief Get the underlying socket (const version)
 * @return Const reference to the tcp_client
 */
const tcp_client &socket_stream::socket() const { return m_socket; }

/**
 * @brief Get the stream buffer
 * @return Reference to socket_stream_buf
 */
socket_stream_buf *socket_stream::rdbuf() const { return m_stream_buf.get(); }

/**
 * @brief Close the underlying socket connection
 */
void socket_stream::close()
{
  if (m_stream_buf)
  {
    m_stream_buf->pubsync(); // Flush pending data
  }
  m_socket.close();
}

/**
 * @brief Check if the underlying socket is connected
 * @return True if connected
 */
bool socket_stream::is_connected() const { return m_socket.is_connected(); }

/**
 * @brief Get local socket address
 * @return Local socket address
 */
socket_address socket_stream::address() const { return m_socket.address(); }

/**
 * @brief Get peer socket address
 * @return Peer socket address
 */
socket_address socket_stream::peer_address() const
{
  return m_socket.peer_address();
}

/**
 * @brief Get current buffer size
 * @return Buffer size in bytes
 */
std::size_t socket_stream::buffer_size() const
{
  return m_stream_buf ? m_stream_buf->buffer_size() : 0;
}

/**
 * @brief Set buffer size (only when stream is empty)
 * @param size New buffer size
 * @throws std::invalid_argument if size is zero
 * @throws std::logic_error if buffered data remains
 */
void socket_stream::set_buffer_size(std::size_t size)
{
  if (m_stream_buf)
  {
    // Flush any pending data before changing buffer size
    flush();
    m_stream_buf->set_buffer_size(size);
  }
}

/**
 * @brief Set TCP no delay option
 * @param flag Enable/disable no delay
 */
void socket_stream::set_no_delay(bool flag) { m_socket.set_no_delay(flag); }

/**
 * @brief Get TCP no delay option
 * @return True if no delay is enabled
 */
bool socket_stream::get_no_delay() { return m_socket.get_no_delay(); }

/**
 * @brief Set keep alive option
 * @param flag Enable/disable keep alive
 */
void socket_stream::set_keep_alive(bool flag) { m_socket.set_keep_alive(flag); }

/**
 * @brief Get keep alive option
 * @return True if keep alive is enabled
 */
bool socket_stream::get_keep_alive() { return m_socket.get_keep_alive(); }

/**
 * @brief Set send timeout
 * @param timeout Send timeout
 */
void socket_stream::set_send_timeout(const std::chrono::milliseconds &timeout)
{
  m_socket.set_send_timeout(timeout);
}

/**
 * @brief Get send timeout
 * @return Send timeout
 */
std::chrono::milliseconds socket_stream::get_send_timeout()
{
  return m_socket.get_send_timeout();
}

/**
 * @brief Set receive timeout
 * @param timeout Receive timeout
 */
void socket_stream::set_receive_timeout(
    const std::chrono::milliseconds &timeout)
{
  m_socket.set_receive_timeout(timeout);
}

/**
 * @brief Get receive timeout
 * @return Receive timeout
 */
std::chrono::milliseconds socket_stream::get_receive_timeout()
{
  return m_socket.get_receive_timeout();
}

} // namespace fb
