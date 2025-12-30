#include <fb/tcp_server.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <system_error>

namespace fb
{

/**
 * @brief Construct an idle server with default limits.
 *
 * The instance starts in an unconfigured state; call `set_server_socket()` and
 * `set_connection_factory()` before invoking `start()`.
 */
tcp_server::tcp_server() :
  m_running(false),
  m_should_stop(false),
  m_max_threads(DEFAULT_MAX_THREADS),
  m_max_queued(DEFAULT_MAX_QUEUED),
  m_connection_timeout(DEFAULT_CONNECTION_TIMEOUT),
  m_idle_timeout(DEFAULT_IDLE_TIMEOUT),
  m_total_connections(0),
  m_has_socket(false),
  m_has_factory(false)
{
}

/**
 * @brief Construct and configure the server.
 *
 * @param server_socket Listening socket that must already be bound and
 * listening.
 * @param connection_factory Factory used to instantiate `tcp_server_connection`
 * objects.
 * @param max_threads Maximum number of worker threads (0 selects
 * DEFAULT_MAX_THREADS).
 * @param max_queued Maximum number of queued connections waiting on workers (0
 * selects DEFAULT_MAX_QUEUED).
 */
tcp_server::tcp_server(fb::server_socket server_socket,
                       connection_factory connection_factory,
                       std::size_t max_threads,
                       std::size_t max_queued) :
  m_server_socket(std::move(server_socket)),
  m_connection_factory(std::move(connection_factory)),
  m_running(false),
  m_should_stop(false),
  m_max_threads(max_threads == 0 ? DEFAULT_MAX_THREADS : max_threads),
  m_max_queued(max_queued == 0 ? DEFAULT_MAX_QUEUED : max_queued),
  m_connection_timeout(DEFAULT_CONNECTION_TIMEOUT),
  m_idle_timeout(DEFAULT_IDLE_TIMEOUT),
  m_total_connections(0),
  m_has_socket(true),
  m_has_factory(true)
{
}

/**
 * @brief Move constructor transfers configuration and runtime state (without
 * worker threads).
 */
tcp_server::tcp_server(tcp_server &&other) noexcept :
  m_server_socket(std::move(other.m_server_socket)),
  m_connection_factory(std::move(other.m_connection_factory)),
  m_running(other.m_running.load()),
  m_should_stop(other.m_should_stop.load()),
  m_max_threads(other.m_max_threads),
  m_max_queued(other.m_max_queued),
  m_connection_timeout(other.m_connection_timeout),
  m_idle_timeout(other.m_idle_timeout),
  m_total_connections(other.m_total_connections.load()),
  m_start_time(other.m_start_time),
  m_has_socket(other.m_has_socket),
  m_has_factory(other.m_has_factory)
{
  // Note: We cannot move threads, so the moved-from server loses its threads
  // This is acceptable as long as it wasn't running
  other.m_running     = false;
  other.m_should_stop = true;
  other.m_has_socket  = false;
  other.m_has_factory = false;
}

/**
 * @brief Move assignment transfers configuration and active state, stopping
 * this server first if needed.
 */
tcp_server &tcp_server::operator=(tcp_server &&other) noexcept
{
  if (this != &other)
  {
    // Stop this server first if running
    if (m_running.load())
    {
      try
      {
        stop();
      }
      catch (...)
      {
        // Ignore exceptions during move assignment cleanup
      }
    }

    m_server_socket      = std::move(other.m_server_socket);
    m_connection_factory = std::move(other.m_connection_factory);
    m_running            = other.m_running.load();
    m_should_stop        = other.m_should_stop.load();
    m_max_threads        = other.m_max_threads;
    m_max_queued         = other.m_max_queued;
    m_connection_timeout = other.m_connection_timeout;
    m_idle_timeout       = other.m_idle_timeout;
    m_total_connections  = other.m_total_connections.load();
    m_start_time         = other.m_start_time;
    m_has_socket         = other.m_has_socket;
    m_has_factory        = other.m_has_factory;

    other.m_running      = false;
    other.m_should_stop  = true;
    other.m_has_socket   = false;
    other.m_has_factory  = false;
  }
  return *this;
}

/**
 * @brief Destructor stops the server if it is still running.
 */
tcp_server::~tcp_server()
{
  try
  {
    if (m_running.load())
    {
      stop();
    }
  }
  catch (...)
  {
    // Ignore exceptions in destructor
  }
}

/**
 * @brief Start accepting client connections and dispatching work to the pool.
 *
 * @throws std::system_error If the listening socket is invalid or closed.
 * @throws std::runtime_error If the server is already running or configuration
 * is incomplete.
 */
void tcp_server::start()
{
  if (m_running.load())
  {
    throw std::runtime_error("tcp_server is already running");
  }

  validate_configuration();

  m_should_stop = false;
  m_start_time  = std::chrono::steady_clock::now();

  try
  {
    on_server_starting();

    // Start acceptor thread
    m_acceptor_thread = std::thread(&tcp_server::acceptor_thread_proc, this);

    // Start initial worker threads
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    for (std::size_t i = 0; i < std::min(m_max_threads, std::size_t(4)); ++i)
    {
      m_worker_threads.emplace_back(&tcp_server::worker_thread_proc, this);
    }

    m_running = true;

    // Emit server started signal
    if (onServerStarted.slot_count() > 0)
    {
      onServerStarted.emit();
    }
  }
  catch (...)
  {
    m_should_stop = true;
    if (m_acceptor_thread.joinable())
    {
      m_acceptor_thread.join();
    }
    throw;
  }
}

/**
 * @brief Stop accepting connections and shut down worker threads.
 *
 * @param timeout Maximum duration to wait for threads and active connections to
 * finish.
 * @throws std::runtime_error If the server is not currently running.
 */
void tcp_server::stop(const std::chrono::milliseconds &timeout)
{
  if (!m_running.load())
  {
    throw std::runtime_error("tcp_server is not running");
  }

  m_should_stop = true;
  m_running     = false;

  // Emit server stopping signal
  if (onServerStopping.slot_count() > 0)
  {
    onServerStopping.emit();
  }

  try
  {
    on_server_stopping();

    // Close server socket to stop accepting connections
    if (!m_server_socket.is_closed())
    {
      m_server_socket.close();
    }

    // Wake up all waiting worker threads
    m_queue_condition.notify_all();

    // Shutdown threads with timeout
    shutdown_threads(timeout);

    // Clean up remaining connections
    cleanup_connections();

    // Emit server stopped signal
    if (onServerStopped.slot_count() > 0)
    {
      onServerStopped.emit();
    }
  }
  catch (const std::exception &ex)
  {
    // Emit exception signal
    if (onException.slot_count() > 0)
    {
      onException.emit(ex, "stop");
    }
    handle_exception(ex, "stop");
    throw;
  }
}

/**
 * @brief Query whether the server is actively running.
 *
 * @return True if the acceptor thread is running and the server has not been
 * stopped.
 */
bool tcp_server::is_running() const { return m_running.load(); }

/**
 * @brief Provide a listening socket to the server.
 *
 * @param server_socket Bound and listening socket instance.
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_server_socket(fb::server_socket server_socket)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set server socket while server is running");
  }

  m_server_socket = std::move(server_socket);
  m_has_socket    = true;
}

/**
 * @brief Install the factory used to create per-connection handlers.
 *
 * @param factory Callable that returns a `tcp_server_connection` for each
 * accepted client.
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_connection_factory(connection_factory factory)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set connection factory while server is running");
  }

  m_connection_factory = std::move(factory);
  m_has_factory        = true;
}

/**
 * @brief Configure the upper bound on worker threads.
 *
 * @param max_threads Maximum worker count (0 applies DEFAULT_MAX_THREADS).
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_max_threads(std::size_t max_threads)
{
  if (m_running.load())
  {
    throw std::runtime_error("Cannot set max threads while server is running");
  }

  m_max_threads = max_threads == 0 ? DEFAULT_MAX_THREADS : max_threads;
}

/**
 * @brief Set the maximum number of pending connections waiting for a worker.
 *
 * @param max_queued Queue capacity (0 applies DEFAULT_MAX_QUEUED).
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_max_queued(std::size_t max_queued)
{
  if (m_running.load())
  {
    throw std::runtime_error("Cannot set max queued while server is running");
  }

  m_max_queued = max_queued == 0 ? DEFAULT_MAX_QUEUED : max_queued;
}

/**
 * @brief Configure the per-connection I/O timeout.
 *
 * @param timeout Duration applied to each `tcp_server_connection` (0 disables
 * the timeout).
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_connection_timeout(
    const std::chrono::milliseconds &timeout)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set connection timeout while server is running");
  }

  m_connection_timeout = timeout;
}

/**
 * @brief Configure automatic shutdown of idle connections.
 *
 * @param timeout Idle timeout; zero disables automatic disconnects.
 * @throws std::runtime_error If called while the server is running.
 */
void tcp_server::set_idle_timeout(const std::chrono::milliseconds &timeout)
{
  if (m_running.load())
  {
    throw std::runtime_error("Cannot set idle timeout while server is running");
  }

  m_idle_timeout = timeout;
}

/**
 * @brief Access the listening socket.
 *
 * @return Reference to the configured `server_socket`.
 * @throws std::runtime_error If no socket has been configured yet.
 */
const fb::server_socket &tcp_server::server_socket() const
{
  if (!m_has_socket)
  {
    throw std::runtime_error("No server socket is set");
  }
  return m_server_socket;
}

/**
 * @brief Get the number of connections currently being serviced.
 *
 * @return Count of active `tcp_server_connection` instances.
 */
std::size_t tcp_server::active_connections() const
{
  std::lock_guard<std::mutex> lock(m_connections_mutex);
  return m_active_connections.size();
}

/**
 * @brief Get the current number of worker threads.
 *
 * @return Size of the worker thread pool.
 */
std::size_t tcp_server::thread_count() const
{
  std::lock_guard<std::mutex> lock(m_threads_mutex);
  return m_worker_threads.size();
}

/**
 * @brief Retrieve the cumulative number of accepted clients.
 *
 * @return Total clients accepted since the server started.
 */
std::uint64_t tcp_server::total_connections() const
{
  return m_total_connections.load();
}

/**
 * @brief Report how many accepted connections are waiting for a worker.
 *
 * @return Pending connection count.
 */
std::size_t tcp_server::queued_connections() const
{
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  return m_connection_queue.size();
}

/**
 * @brief Measure how long the server has been running.
 *
 * @return Duration since `start()` was called, or zero if not running.
 */
std::chrono::steady_clock::duration tcp_server::uptime() const
{
  if (!m_running.load())
  {
    return std::chrono::steady_clock::duration::zero();
  }
  return std::chrono::steady_clock::now() - m_start_time;
}

/**
 * @brief Default exception handling hook for subclasses.
 *
 * @param ex Exception that was thrown.
 * @param context String describing where the exception occurred.
 *
 * The base implementation is intentionally minimal; override to provide logging
 * or alerts.
 */
void tcp_server::handle_exception(const std::exception &ex,
                                  const std::string &context) noexcept
{
  static_cast<void>(ex);
  static_cast<void>(context);
  // Default implementation: In production, this would log the exception
  // For now, we just continue operation
  // LOG_ERROR("tcp_server exception in " << context << ": " << ex.what());
}

/**
 * @brief Hook invoked immediately before worker threads are launched.
 *
 * Override to perform initialization tasks. The base implementation is a no-op.
 */
void tcp_server::on_server_starting()
{
  // Default implementation does nothing
}

/**
 * @brief Hook invoked after `stop()` has been requested but before resources
 * are released.
 *
 * Override to perform cleanup tasks. The base implementation is a no-op.
 */
void tcp_server::on_server_stopping()
{
  // Default implementation does nothing
}

/**
 * @brief Main loop for the acceptor thread responsible for admitting new
 * clients.
 */
void tcp_server::acceptor_thread_proc()
{
  while (!m_should_stop.load())
  {
    try
    {
      socket_address client_address;

      // Accept connection with timeout to allow checking stop condition
      tcp_client client_socket = m_server_socket.accept_connection(
          client_address, std::chrono::milliseconds(1000));

      if (m_should_stop.load())
      {
        break;
      }

      // Create connection handler
      auto connection =
          m_connection_factory(std::move(client_socket), client_address);
      if (!connection)
      {
        continue; // Factory returned null, skip this connection
      }

      // Queue connection for processing
      {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        if (m_connection_queue.size() >= m_max_queued)
        {
          // Queue is full, drop connection
          continue;
        }
        m_connection_queue.push(std::move(connection));
      }

      // Notify worker threads and possibly add more threads
      m_queue_condition.notify_one();
      add_worker_thread_if_needed();

      m_total_connections.fetch_add(1);
    }
    catch (const std::system_error &ex)
    {
      if (ex.code() == std::make_error_code(std::errc::timed_out))
      {
        // Timeout during accept is normal, continue loop
        continue;
      }
      handle_exception(ex, "acceptor_thread");
      if (m_server_socket.is_closed())
      {
        break;
      }
      continue;
    }
    catch (const std::exception &ex)
    {
      handle_exception(ex, "acceptor_thread");
      if (m_server_socket.is_closed())
      {
        break; // Server socket is closed, stop accepting
      }
    }
  }
}

/**
 * @brief Worker loop that pulls connections from the queue and processes them.
 */
void tcp_server::worker_thread_proc()
{
  while (!m_should_stop.load())
  {
    std::unique_ptr<tcp_server_connection> connection;

    // Wait for connection to process
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);
      m_queue_condition.wait_for(
          lock, std::chrono::seconds(1), [this]
          { return !m_connection_queue.empty() || m_should_stop.load(); });

      if (m_should_stop.load())
      {
        break;
      }

      if (m_connection_queue.empty())
      {
        continue;
      }

      connection = std::move(m_connection_queue.front());
      m_connection_queue.pop();
    }

    if (connection)
    {
      process_connection(std::move(connection));
    }
  }
}

/**
 * @brief Execute the lifecycle of a single accepted connection.
 *
 * @param connection Newly accepted connection to process.
 */
void tcp_server::process_connection(
    std::unique_ptr<tcp_server_connection> connection)
{
  try
  {
    // Set connection timeout if configured
    if (m_connection_timeout.count() > 0)
    {
      connection->set_timeout(m_connection_timeout);
    }

    // Add to active connections for tracking
    std::shared_ptr<tcp_server_connection> shared_connection(
        connection.release());
    {
      std::lock_guard<std::mutex> lock(m_connections_mutex);
      m_active_connections.push_back(shared_connection);
    }

    // Process the connection
    shared_connection->start();
  }
  catch (const std::exception &ex)
  {
    handle_exception(ex, "process_connection");
  }

  // Cleanup finished connections periodically
  cleanup_connections();
}

/**
 * @brief Remove completed or disconnected connections from the active list.
 */
void tcp_server::cleanup_connections()
{
  std::lock_guard<std::mutex> lock(m_connections_mutex);

  m_active_connections.erase(
      std::remove_if(m_active_connections.begin(), m_active_connections.end(),
                     [](const std::shared_ptr<tcp_server_connection> &conn)
                     { return !conn || !conn->is_connected(); }),
      m_active_connections.end());
}

/**
 * @brief Grow the worker pool when the backlog exceeds current capacity.
 */
void tcp_server::add_worker_thread_if_needed()
{
  std::lock_guard<std::mutex> threads_lock(m_threads_mutex);
  std::lock_guard<std::mutex> queue_lock(m_queue_mutex);

  // Add thread if queue is getting full and we haven't reached max threads
  if (m_connection_queue.size() > m_worker_threads.size() &&
      m_worker_threads.size() < m_max_threads)
  {
    m_worker_threads.emplace_back(&tcp_server::worker_thread_proc, this);
  }
}

/**
 * @brief Ensure the server has the prerequisites needed to start.
 *
 * @throws std::system_error If the listening socket is closed.
 * @throws std::runtime_error If configuration is incomplete.
 */
void tcp_server::validate_configuration() const
{
  if (!is_configured())
  {
    throw std::runtime_error("tcp_server is not properly configured");
  }

  if (m_server_socket.is_closed())
  {
    throw std::logic_error("Server socket is closed or invalid");
  }

  if (!m_connection_factory)
  {
    throw std::runtime_error("Connection factory is not set");
  }
}

/**
 * @brief Join the acceptor and worker threads, respecting a timeout.
 *
 * @param timeout Maximum time allotted for graceful shutdown.
 */
void tcp_server::shutdown_threads(const std::chrono::milliseconds &timeout)
{
  auto deadline = std::chrono::steady_clock::now() + timeout;

  // Signal active connections to stop before joining worker threads so they
  // can exit promptly.
  {
    std::lock_guard<std::mutex> lock(m_connections_mutex);
    for (auto &conn : m_active_connections)
    {
      if (conn)
      {
        conn->stop();
      }
    }
  }

  // Join acceptor thread
  if (m_acceptor_thread.joinable())
  {
    m_acceptor_thread.join();
  }

  // Join worker threads
  {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    for (auto &thread : m_worker_threads)
    {
      if (thread.joinable())
      {
        // Calculate remaining time
        auto now = std::chrono::steady_clock::now();
        if (now < deadline)
        {
          thread.join();
        }
        else
        {
          // Force detach if timeout exceeded
          thread.detach();
        }
      }
    }
    m_worker_threads.clear();
  }

  // Remove any completed connections after threads have exited
  cleanup_connections();
}

/**
 * @brief Check whether both a listening socket and factory have been provided.
 *
 * @return True if the server can be started.
 */
bool tcp_server::is_configured() const { return m_has_socket && m_has_factory; }

} // namespace fb
