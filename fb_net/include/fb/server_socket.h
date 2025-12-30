#pragma once

#include <fb/socket_base.h>
#include <fb/tcp_client.h>
#include <fb/socket_address.h>

namespace fb {

/**
 * @brief TCP server socket implementation
 * This class inherits from socket_base and adds TCP-specific server functionality.
 */
class server_socket : public socket_base
{
public:

  server_socket();

  explicit server_socket(socket_address::Family family);
  explicit server_socket(std::uint16_t port, int backlog = 64);
  explicit server_socket(const socket_address& address, int backlog = 64);

  server_socket(const socket_address& address, int backlog, bool reuse_address);
  server_socket(server_socket&& other) noexcept;
  server_socket(const server_socket&) = delete;
  server_socket& operator=(const server_socket&) = delete;
  server_socket& operator=(server_socket&& other) noexcept;

  virtual ~server_socket() = default;

  // Server socket operations

  void bind(const socket_address& address, bool reuse_address = true);
  void bind(const socket_address& address, bool reuse_address, bool reuse_port);
  void listen(int backlog = 64);

  tcp_client accept_connection();
  tcp_client accept_connection(socket_address& client_address);
  tcp_client accept_connection(const std::chrono::milliseconds& timeout);
  tcp_client accept_connection(socket_address& client_address,
                               const std::chrono::milliseconds& timeout);



  void set_backlog(int backlog);
  void set_reuse_address(bool flag);
  bool get_reuse_address();
  void set_reuse_port(bool flag);
  bool get_reuse_port();

  bool has_pending_connections(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(0));
  int max_connections() const;
  int queue_size() const;

private:

    void init_server_socket(socket_address::Family family);

    void setup_server(const socket_address& address,
                      int backlog,
                      bool reuse_address,
                      bool reuse_port = false);

    int m_backlog;
    static constexpr int DEFAULT_BACKLOG = 64;
    static constexpr int MAX_BACKLOG = 1024;
};

} // namespace fb
