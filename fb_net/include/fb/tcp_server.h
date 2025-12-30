#pragma once

#include <fb/server_socket.h>
#include <fb/tcp_server_connection.h>
#include <fb/socket_address.h>
#include <fb/fb_signal.hpp>
#include <memory>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>

namespace fb {

/**
 * @class fb::tcp_server
 * @brief Multi-threaded TCP server that accepts connections and dispatches them to worker threads.
 *
 * The server owns a listening `server_socket`, accepts clients, and hands them off to
 * `tcp_server_connection` handlers managed by a small worker pool. It tracks connections,
 * manages graceful shutdown, and exposes basic runtime statistics.
 */
class tcp_server
{
public:

    /**
     * @brief Connection factory function type for creating tcp_server_connection instances
     * @param socket StreamSocket for the accepted connection
     * @param client_address Address of the connected client
     * @return Unique pointer to TCPServerConnection-derived instance
     */
    using connection_factory = std::function<std::unique_ptr<tcp_server_connection>(tcp_client, const socket_address&)>;

    tcp_server();
    tcp_server(fb::server_socket server_socket, 
              connection_factory connection_factory,
              std::size_t max_threads = 0,
              std::size_t max_queued = 0);

    tcp_server(const tcp_server&) = delete;
    tcp_server(tcp_server&& other) noexcept;
    tcp_server& operator=(const tcp_server&) = delete;
    tcp_server& operator=(tcp_server&& other) noexcept;
    virtual ~tcp_server();

    // Server lifecycle management

    void start();
    void stop(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(5000));
    bool is_running() const;

    // Server configuration

    void set_server_socket(fb::server_socket server_socket);
    void set_connection_factory(connection_factory factory);
    void set_max_threads(std::size_t max_threads);
    void set_max_queued(std::size_t max_queued);
    void set_connection_timeout(const std::chrono::milliseconds& timeout);
    void set_idle_timeout(const std::chrono::milliseconds& timeout);

    // Server status and statistics

    const fb::server_socket& server_socket() const;
    std::size_t active_connections() const;
    std::size_t thread_count() const;
    std::uint64_t total_connections() const;
    std::size_t queued_connections() const;
    std::chrono::steady_clock::duration uptime() const;

    // Signals for server events
    // Note: Signals are emitted on the acceptor/worker threads
    fb::signal<> onServerStarted;                              ///< Emitted when server starts
    fb::signal<> onServerStopping;                             ///< Emitted when server begins shutdown
    fb::signal<> onServerStopped;                              ///< Emitted when server has stopped
    fb::signal<const socket_address&> onConnectionAccepted;    ///< Emitted when new connection accepted
    fb::signal<const socket_address&> onConnectionClosed;      ///< Emitted when connection closes
    fb::signal<size_t> onActiveConnectionsChanged;             ///< Emitted when active connection count changes
    fb::signal<const std::exception&, const std::string&> onException;  ///< Emitted when exception occurs

protected:

  virtual void handle_exception(const std::exception& ex, const std::string& context) noexcept;
  virtual void on_server_starting();
  virtual void on_server_stopping();

private:

    // Server state
    fb::server_socket m_server_socket;
    connection_factory m_connection_factory;
    std::atomic<bool> m_running;
    std::atomic<bool> m_should_stop;
    
    // Threading infrastructure
    std::thread m_acceptor_thread;
    std::vector<std::thread> m_worker_threads;
    std::queue<std::unique_ptr<tcp_server_connection>> m_connection_queue;
    mutable std::mutex m_queue_mutex;
    std::condition_variable m_queue_condition;
    mutable std::mutex m_threads_mutex;
    
    // Active connections tracking
    std::vector<std::shared_ptr<tcp_server_connection>> m_active_connections;
    mutable std::mutex m_connections_mutex;
    
    // Configuration
    std::size_t m_max_threads;
    std::size_t m_max_queued;
    std::chrono::milliseconds m_connection_timeout;
    std::chrono::milliseconds m_idle_timeout;
    
    // Statistics
    std::atomic<std::uint64_t> m_total_connections;
    std::chrono::steady_clock::time_point m_start_time;
    
    // Configuration validation
    bool m_has_socket;
    bool m_has_factory;
    
    // Thread pool management
    static constexpr std::size_t DEFAULT_MAX_THREADS = 100;
    static constexpr std::size_t DEFAULT_MAX_QUEUED = 100;
    static constexpr auto DEFAULT_CONNECTION_TIMEOUT = std::chrono::milliseconds(30000);
    static constexpr auto DEFAULT_IDLE_TIMEOUT = std::chrono::milliseconds(0);

    void acceptor_thread_proc();
    void worker_thread_proc();
    void process_connection(std::unique_ptr<tcp_server_connection> connection);
    void cleanup_connections();
    void add_worker_thread_if_needed();
    void validate_configuration() const;
    void shutdown_threads(const std::chrono::milliseconds& timeout);
    bool is_configured() const;
};

} // namespace fb
