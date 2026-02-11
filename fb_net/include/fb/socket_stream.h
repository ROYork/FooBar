#pragma once

#include <fb/tcp_client.h>
#include <iostream>
#include <streambuf>
#include <memory>

namespace fb {

/**
 * @brief Stream buffer class for socket I/O operations.
 * This class provides the underlying buffer management for socket_stream.
 */
class socket_stream_buf : public std::streambuf
{
public:

  explicit socket_stream_buf(tcp_client& socket, std::size_t buffer_size = 8192);
  virtual ~socket_stream_buf();
  socket_stream_buf(const socket_stream_buf&) = delete;
  socket_stream_buf& operator=(const socket_stream_buf&) = delete;
  tcp_client& socket();
  std::size_t buffer_size() const;
  void set_buffer_size(std::size_t size);

protected:

  // std::streambuf overrides for input operations
  virtual int_type underflow() override;
  virtual int_type uflow() override;
  virtual std::streamsize showmanyc() override;

  // std::streambuf overrides for output operations
  virtual int_type overflow(int_type c = traits_type::eof()) override;
  virtual int sync() override;

  // std::streambuf overrides for buffer management
  virtual std::streambuf* setbuf(char* s, std::streamsize n) override;

private:

  void init_input_buffer();
  void init_output_buffer();
  int flush_output_buffer();
  int fill_input_buffer();

  tcp_client& m_socket;
  std::size_t m_buffer_size;

  // Input buffer management
  std::unique_ptr<char[]> m_input_buffer;
  bool m_input_buffer_initialized;

  // Output buffer management
  std::unique_ptr<char[]> m_output_buffer;
  bool m_output_buffer_initialized;
};


/**
 * @brief Stream-based socket I/O interface providing std::iostream compatibility.
 * This class wraps tcp_client to provide formatted input/output operations
 * using standard C++ stream operators and methods.
 */
class socket_stream : public std::iostream
{
public:

  explicit socket_stream(tcp_client& socket, std::size_t buffer_size = 8192);

  explicit socket_stream(const socket_address& address, std::size_t buffer_size = 8192);

  socket_stream(const socket_address& address,
               const std::chrono::milliseconds& timeout,
               std::size_t buffer_size = 8192);

  virtual ~socket_stream();

  socket_stream(const socket_stream&) = delete;

  socket_stream& operator=(const socket_stream&) = delete;

  tcp_client& socket();

  const tcp_client& socket() const;

  socket_stream_buf* rdbuf() const;

  void close();

  bool is_connected() const;

  socket_address address() const;

  socket_address peer_address() const;

  std::size_t buffer_size() const;

  void set_buffer_size(std::size_t size);
  void set_no_delay(bool flag);
  bool get_no_delay();
  void set_keep_alive(bool flag);
  bool get_keep_alive();
  void set_send_timeout(const std::chrono::milliseconds& timeout);

  std::chrono::milliseconds get_send_timeout();

  void set_receive_timeout(const std::chrono::milliseconds& timeout);

  std::chrono::milliseconds get_receive_timeout();

private:

  std::unique_ptr<tcp_client> m_owned_socket;  // For sockets we create
  tcp_client& m_socket;                        // Reference to socket (owned or external)
  std::unique_ptr<socket_stream_buf> m_stream_buf;
};

} // namespace fb
