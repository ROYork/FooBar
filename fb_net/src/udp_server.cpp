#include <fb/udp_server.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <system_error>

namespace fb
{

/**
 * @class fb::udp_server
 * @brief Multi-threaded UDP server with worker-based packet processing.
 *
 * The server owns a listening `udp_socket`, receives datagrams, enqueues them,
 * and hands them off to `udp_handler` instances supplied via a factory or a
 * shared handler. It manages worker threads, lifecycle hooks, and statistics.
 */

/**
 * @brief Construct an idle server with default configuration.
 *
 * The instance starts unconfigured; set a socket and handler factory before
 * calling `start()`.
 */
udp_server::udp_server() :
  m_running(false),
  m_should_stop(false),
  m_max_threads(DEFAULT_MAX_THREADS),
  m_max_queued(DEFAULT_MAX_QUEUED),
  m_packet_buffer_size(DEFAULT_PACKET_BUFFER_SIZE),
  m_packet_timeout(DEFAULT_PACKET_TIMEOUT),
  m_total_packets(0),
  m_processed_packets(0),
  m_dropped_packets(0),
  m_has_socket(false),
  m_has_handler(false)
{
}

/**
 * @brief Construct a server that creates a handler per packet.
 *
 * @param server_socket Bound socket used to receive datagrams.
 * @param handler_factory Factory invoked for each received packet.
 * @param max_threads Maximum worker threads (0 selects DEFAULT_MAX_THREADS).
 * @param max_queued Maximum queued packets (0 selects DEFAULT_MAX_QUEUED).
 */
udp_server::udp_server(udp_socket server_socket,
                       HandlerFactory handler_factory,
                       std::size_t max_threads,
                       std::size_t max_queued) :
  m_server_socket(std::move(server_socket)),
  m_handler_factory(std::move(handler_factory)),
  m_running(false),
  m_should_stop(false),
  m_max_threads(max_threads == 0 ? DEFAULT_MAX_THREADS : max_threads),
  m_max_queued(max_queued == 0 ? DEFAULT_MAX_QUEUED : max_queued),
  m_packet_buffer_size(DEFAULT_PACKET_BUFFER_SIZE),
  m_packet_timeout(DEFAULT_PACKET_TIMEOUT),
  m_total_packets(0),
  m_processed_packets(0),
  m_dropped_packets(0),
  m_has_socket(true),
  m_has_handler(true)
{
}

/**
 * @brief Construct a server that reuses a shared handler instance.
 *
 * @param server_socket Bound socket used to receive datagrams.
 * @param handler Shared handler invoked for every packet.
 * @param max_threads Maximum worker threads (0 selects DEFAULT_MAX_THREADS).
 * @param max_queued Maximum queued packets (0 selects DEFAULT_MAX_QUEUED).
 */
udp_server::udp_server(udp_socket server_socket,
                       std::shared_ptr<udp_handler> handler,
                       std::size_t max_threads,
                       std::size_t max_queued) :
  m_server_socket(std::move(server_socket)),
  m_shared_handler(handler),
  m_handler_factory(create_shared_handler_factory(handler)),
  m_running(false),
  m_should_stop(false),
  m_max_threads(max_threads == 0 ? DEFAULT_MAX_THREADS : max_threads),
  m_max_queued(max_queued == 0 ? DEFAULT_MAX_QUEUED : max_queued),
  m_packet_buffer_size(DEFAULT_PACKET_BUFFER_SIZE),
  m_packet_timeout(DEFAULT_PACKET_TIMEOUT),
  m_total_packets(0),
  m_processed_packets(0),
  m_dropped_packets(0),
  m_has_socket(true),
  m_has_handler(true)
{
}

/**
 * @brief Move constructor transfers ownership of server resources.
 *
 * Threads are not transferred; the moved-from server is left stopped and
 * unconfigured.
 */
udp_server::udp_server(udp_server &&other) noexcept :
  m_server_socket(std::move(other.m_server_socket)),
  m_shared_handler(std::move(other.m_shared_handler)),
  m_handler_factory(std::move(other.m_handler_factory)),
  m_running(other.m_running.load()),
  m_should_stop(other.m_should_stop.load()),
  m_max_threads(other.m_max_threads),
  m_max_queued(other.m_max_queued),
  m_packet_buffer_size(other.m_packet_buffer_size),
  m_packet_timeout(other.m_packet_timeout),
  m_total_packets(other.m_total_packets.load()),
  m_processed_packets(other.m_processed_packets.load()),
  m_dropped_packets(other.m_dropped_packets.load()),
  m_start_time(other.m_start_time),
  m_has_socket(other.m_has_socket),
  m_has_handler(other.m_has_handler)
{
  // Note: We cannot move threads, so the moved-from server loses its threads
  other.m_running     = false;
  other.m_should_stop = true;
  other.m_has_socket  = false;
  other.m_has_handler = false;
}

/**
 * @brief Move assignment transfers configuration and runtime state.
 *
 * The current server is stopped if necessary. Threads are not migrated;
 * the source instance is left stopped and unconfigured.
 *
 * @return Reference to this instance.
 */
udp_server &udp_server::operator=(udp_server &&other) noexcept
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
    m_handler_factory    = std::move(other.m_handler_factory);
    m_shared_handler     = std::move(other.m_shared_handler);
    m_running            = other.m_running.load();
    m_should_stop        = other.m_should_stop.load();
    m_max_threads        = other.m_max_threads;
    m_max_queued         = other.m_max_queued;
    m_packet_buffer_size = other.m_packet_buffer_size;
    m_packet_timeout     = other.m_packet_timeout;
    m_total_packets      = other.m_total_packets.load();
    m_processed_packets  = other.m_processed_packets.load();
    m_dropped_packets    = other.m_dropped_packets.load();
    m_start_time         = other.m_start_time;
    m_has_socket         = other.m_has_socket;
    m_has_handler        = other.m_has_handler;

    other.m_running      = false;
    other.m_should_stop  = true;
    other.m_has_socket   = false;
    other.m_has_handler  = false;
  }
  return *this;
}

/**
 * @brief Destructor stops the server if it is still running.
 */
udp_server::~udp_server()
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
 * @brief Start receiving packets and dispatching them to workers.
 *
 * @throws std::system_error If the server socket is invalid.
 * @throws std::runtime_error If already running or configuration is incomplete.
 */
void udp_server::start()
{
  if (m_running.load())
  {
    throw std::runtime_error("udp_server is already running");
  }

  validate_configuration();

  m_should_stop = false;
  m_start_time  = std::chrono::steady_clock::now();

  try
  {
    on_server_starting();

    // Start receiver thread
    m_receiver_thread = std::thread(&udp_server::receiver_thread_proc, this);

    // Start initial worker threads (emit signals outside lock)
    std::vector<std::size_t> created_counts;
    {
      std::lock_guard<std::mutex> lock(m_threads_mutex);
      for (std::size_t i = 0; i < std::min(m_max_threads, std::size_t(2)); ++i)
      {
        m_worker_threads.emplace_back(&udp_server::worker_thread_proc, this);
        created_counts.push_back(m_worker_threads.size());
      }
    }
    if (onWorkerThreadCreated.slot_count() > 0)
    {
      for (auto count : created_counts)
      {
        onWorkerThreadCreated.emit(count);
      }
    }

    m_running = true;

    // Emit server started signal
    if (onServerStarted.slot_count() > 0) {
      onServerStarted.emit();
    }
  }
  catch (...)
  {
    m_should_stop = true;
    if (m_receiver_thread.joinable())
    {
      m_receiver_thread.join();
    }
    throw;
  }
}

/**
 * @brief Stop the server and drain worker threads.
 *
 * @param timeout Maximum duration to wait for graceful shutdown.
 * @throws std::runtime_error If the server is not running.
 */
void udp_server::stop(const std::chrono::milliseconds &timeout)
{
  if (!m_running.load())
  {
    throw std::runtime_error("udp_server is not running");
  }

  m_should_stop = true;
  m_running     = false;

  try
  {
    // Emit stopping signal
    if (onServerStopping.slot_count() > 0) {
      onServerStopping.emit();
    }

    on_server_stopping();

    // Close server socket to stop receiving packets
    if (!m_server_socket.is_closed())
    {
      m_server_socket.close();
    }

    // Wake up all waiting worker threads
    m_queue_condition.notify_all();

    // Shutdown threads with timeout
    shutdown_threads(timeout);

    // Clear any remaining packets
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      while (!m_packet_queue.empty())
      {
        m_packet_queue.pop();
      }
    }

    // Emit stopped signal
    if (onServerStopped.slot_count() > 0) {
      onServerStopped.emit();
    }
  }
  catch (const std::exception &ex)
  {
    if (onException.slot_count() > 0) {
      onException.emit(ex, "stop");
    }
    handle_exception(ex, "stop");
    throw;
  }
}

/**
 * @brief Check whether the server is currently running.
 *
 * @return True if the receiver thread is active.
 */
bool udp_server::is_running() const { return m_running.load(); }

/**
 * @brief Provide the socket that will receive packets.
 *
 * @param server_socket Bound `udp_socket`.
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_server_socket(udp_socket server_socket)
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
 * @brief Install the factory used to create per-packet handlers.
 *
 * @param factory Function returning a new `udp_handler` for each packet.
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_handler_factory(HandlerFactory factory)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set handler factory while server is running");
  }

  m_handler_factory = std::move(factory);
  m_shared_handler.reset();
  m_has_handler = true;
}

/**
 * @brief Provide a shared handler instance used for every packet.
 *
 * @param handler Shared handler implementation.
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_shared_handler(std::shared_ptr<udp_handler> handler)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set shared handler while server is running");
  }

  m_shared_handler  = handler;
  m_handler_factory = create_shared_handler_factory(handler);
  m_has_handler     = true;
}

/**
 * @brief Configure the maximum number of worker threads.
 *
 * @param max_threads Upper bound (0 selects DEFAULT_MAX_THREADS).
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_max_threads(std::size_t max_threads)
{
  if (m_running.load())
  {
    throw std::runtime_error("Cannot set max threads while server is running");
  }

  m_max_threads = max_threads == 0 ? DEFAULT_MAX_THREADS : max_threads;
}

/**
 * @brief Configure the maximum backlog of queued packets.
 *
 * @param max_queued Queue capacity (0 selects DEFAULT_MAX_QUEUED).
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_max_queued(std::size_t max_queued)
{
  if (m_running.load())
  {
    throw std::runtime_error("Cannot set max queued while server is running");
  }

  m_max_queued = max_queued == 0 ? DEFAULT_MAX_QUEUED : max_queued;
}

/**
 * @brief Configure the receive buffer size for incoming packets.
 *
 * @param size Buffer size in bytes.
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_packet_buffer_size(std::size_t size)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set packet buffer size while server is running");
  }

  m_packet_buffer_size = size;
}

/**
 * @brief Configure the maximum age for queued packets.
 *
 * @param timeout Packet timeout (0 disables expiration).
 * @throws std::runtime_error If the server is already running.
 */
void udp_server::set_packet_timeout(const std::chrono::milliseconds &timeout)
{
  if (m_running.load())
  {
    throw std::runtime_error(
        "Cannot set packet timeout while server is running");
  }

  m_packet_timeout = timeout;
}

/**
 * @brief Access the configured server socket.
 *
 * @return Reference to the listening `udp_socket`.
 * @throws std::runtime_error If no socket has been configured.
 */
const udp_socket &udp_server::server_socket() const
{
  if (!m_has_socket)
  {
    throw std::runtime_error("No server socket is set");
  }
  return m_server_socket;
}

/**
 * @brief Report the number of worker threads currently running.
 *
 * @return Worker thread count.
 */
std::size_t udp_server::thread_count() const
{
  std::lock_guard<std::mutex> lock(m_threads_mutex);
  return m_worker_threads.size();
}

/**
 * @brief Total number of packets received since the server started.
 */
std::uint64_t udp_server::total_packets() const
{
  return m_total_packets.load();
}

/**
 * @brief Total number of packets processed successfully.
 */
std::uint64_t udp_server::processed_packets() const
{
  return m_processed_packets.load();
}

/**
 * @brief Total number of packets dropped (queue overflow, timeout, etc.).
 */
std::uint64_t udp_server::dropped_packets() const
{
  return m_dropped_packets.load();
}

/**
 * @brief Number of packets waiting in the processing queue.
 *
 * @return Current queue depth.
 */
std::size_t udp_server::queued_packets() const
{
  std::lock_guard<std::mutex> lock(m_queue_mutex);
  return m_packet_queue.size();
}

/**
 * @brief Duration the server has been running.
 *
 * @return Elapsed time since `start()`, or zero if not running.
 */
std::chrono::steady_clock::duration udp_server::uptime() const
{
  if (!m_running.load())
  {
    return std::chrono::steady_clock::duration::zero();
  }
  return std::chrono::steady_clock::now() - m_start_time;
}

/**
 * @brief Default exception handler for server operations.
 *
 * @param ex Exception encountered.
 * @param context Label describing where the exception was raised.
 *
 * The base implementation is intentionally quiet; override to log or alert.
 */
void udp_server::handle_exception(const std::exception &ex,
                                  const std::string &context) noexcept
{
  // Default implementation: In production, this would log the exception
  // For now, we just continue operation
  // LOG_ERROR("udp_server exception in " << context << ": " << ex.what());
  (void)ex;      // Avoid unused parameter warning
  (void)context; // Avoid unused parameter warning
}

/**
 * @brief Hook invoked before worker threads are launched.
 *
 * Override to perform initialisation tasks. The base implementation is a no-op.
 */
void udp_server::on_server_starting()
{
  // Default implementation does nothing
}

/**
 * @brief Hook invoked after a stop has been requested.
 *
 * Override to perform cleanup tasks. The base implementation is a no-op.
 */
void udp_server::on_server_stopping()
{
  // Default implementation does nothing
}

/**
 * @brief Main loop for the receiver thread.
 *
 * Accepts datagrams, enqueues them, and triggers worker wake-ups while
 * honouring stop requests and queue limits.
 */
void udp_server::receiver_thread_proc()
{
  std::vector<std::uint8_t> buffer(m_packet_buffer_size);

  while (!m_should_stop.load())
  {
    try
    {
      socket_address sender_address;

      // Poll for readability so we can check stop condition without
      // mutating socket timeouts configured by the caller.
      if (!m_server_socket.poll_read(std::chrono::milliseconds(1000)))
      {
        continue;
      }

      int received = m_server_socket.receive_from(
          buffer.data(), static_cast<int>(buffer.size()), sender_address);

      if (m_should_stop.load())
      {
        break;
      }

      if (received > 0)
      {
        // Increment total_packets for ALL received packets (including dropped ones)
        auto total_count = m_total_packets.fetch_add(1) + 1;

        // Emit packet received signal (outside any locks)
        if (onPacketReceived.slot_count() > 0) {
          onPacketReceived.emit(buffer.data(), static_cast<std::size_t>(received), sender_address);
        }
        if (onTotalPacketsChanged.slot_count() > 0) {
          onTotalPacketsChanged.emit(total_count);
        }

        // Create packet data
        auto packet_data = std::make_unique<PacketData>(
            buffer.data(), static_cast<std::size_t>(received), sender_address);

        // Queue packet for processing - signals emitted outside lock to avoid deadlock
        bool packet_dropped = false;
        std::size_t queue_size = 0;
        {
          std::lock_guard<std::mutex> lock(m_queue_mutex);
          if (m_packet_queue.size() >= m_max_queued)
          {
            // Queue is full, drop packet
            m_dropped_packets.fetch_add(1);
            packet_dropped = true;
          }
          else
          {
            m_packet_queue.push(std::move(packet_data));
            queue_size = m_packet_queue.size();
          }
        }

        // Emit signals outside the lock to prevent re-entrancy deadlock
        if (packet_dropped)
        {
          if (onDroppedPacketsChanged.slot_count() > 0) {
            onDroppedPacketsChanged.emit(m_dropped_packets.load());
          }
          continue;
        }

        if (onQueuedPacketsChanged.slot_count() > 0) {
          onQueuedPacketsChanged.emit(queue_size);
        }

        // Notify worker threads and possibly add more threads
        m_queue_condition.notify_one();
        add_worker_thread_if_needed();
      }
    }
    catch (const std::system_error &ex)
    {
      if (ex.code() == std::make_error_code(std::errc::timed_out))
      {
        // Timeout during receive is normal, continue loop
        continue;
      }
      handle_exception(ex, "receiver_thread");
      if (m_server_socket.is_closed())
      {
        break;
      }
      continue;
    }
    catch (const std::exception &ex)
    {
      handle_exception(ex, "receiver_thread");
      if (m_server_socket.is_closed())
      {
        break; // Server socket is closed, stop receiving
      }
    }
  }
}

/**
 * @brief Worker loop that drains the packet queue.
 */
void udp_server::worker_thread_proc()
{
  while (!m_should_stop.load())
  {
    std::unique_ptr<PacketData> packet_data;

    // Wait for packet to process
    {
      std::unique_lock<std::mutex> lock(m_queue_mutex);
      m_queue_condition.wait_for(
          lock, std::chrono::seconds(1),
          [this] { return !m_packet_queue.empty() || m_should_stop.load(); });

      if (m_should_stop.load())
      {
        break;
      }

      if (m_packet_queue.empty())
      {
        continue;
      }

      packet_data = std::move(m_packet_queue.front());
      m_packet_queue.pop();
    }

    if (packet_data)
    {
      process_packet(std::move(packet_data));
    }

    // Cleanup expired packets periodically
    cleanup_expired_packets();
  }
}

/**
 * @brief Process a single queued packet through a handler.
 *
 * @param packet_data Packet removed from the queue.
 */
void udp_server::process_packet(std::unique_ptr<PacketData> packet_data)
{
  try
  {
    // Check if packet has expired
    if (m_packet_timeout.count() > 0)
    {
      auto age = std::chrono::steady_clock::now() - packet_data->received_time;
      if (age > m_packet_timeout)
      {
        m_dropped_packets.fetch_add(1);
        return;
      }
    }

    bool success = false;

    // Use shared handler if available, otherwise create new handler
    if (m_shared_handler)
    {
      success = m_shared_handler->process_packet(packet_data->buffer.data(),
                                                 packet_data->buffer.size(),
                                                 packet_data->sender_address);
    }
    else
    {
      // Create handler for this packet
      auto handler = m_handler_factory(*packet_data);
      if (!handler)
      {
        // Factory returned null, skip this packet
        m_dropped_packets.fetch_add(1);
        return;
      }

      // Process the packet
      success = handler->process_packet(packet_data->buffer.data(),
                                        packet_data->buffer.size(),
                                        packet_data->sender_address);
    }

    if (success)
    {
      auto processed_count = m_processed_packets.fetch_add(1) + 1;
      if (onProcessedPacketsChanged.slot_count() > 0) {
        onProcessedPacketsChanged.emit(processed_count);
      }
    }
    else
    {
      auto dropped_count = m_dropped_packets.fetch_add(1) + 1;
      if (onDroppedPacketsChanged.slot_count() > 0) {
        onDroppedPacketsChanged.emit(dropped_count);
      }
    }
  }
  catch (const std::exception &ex)
  {
    if (onException.slot_count() > 0) {
      onException.emit(ex, "process_packet");
    }
    handle_exception(ex, "process_packet");
    auto dropped_count = m_dropped_packets.fetch_add(1) + 1;
    if (onDroppedPacketsChanged.slot_count() > 0) {
      onDroppedPacketsChanged.emit(dropped_count);
    }
  }
}

/**
 * @brief Remove packets from the queue that have exceeded the timeout.
 */
void udp_server::cleanup_expired_packets()
{
  if (m_packet_timeout.count() == 0)
  {
    return; // No timeout configured
  }

  std::lock_guard<std::mutex> lock(m_queue_mutex);
  auto now = std::chrono::steady_clock::now();

  while (!m_packet_queue.empty())
  {
    auto &front_packet = m_packet_queue.front();
    auto age           = now - front_packet->received_time;

    if (age > m_packet_timeout)
    {
      m_packet_queue.pop();
      m_dropped_packets.fetch_add(1);
    }
    else
    {
      break; // Packets are ordered by arrival time
    }
  }
}

/**
 * @brief Spawn an additional worker when the queue pressure increases.
 */
void udp_server::add_worker_thread_if_needed()
{
  bool thread_added = false;
  std::size_t thread_count = 0;

  // Use scoped_lock for deadlock-safe acquisition of multiple mutexes
  {
    std::scoped_lock lock(m_threads_mutex, m_queue_mutex);

    // Add thread if queue is getting full and we haven't reached max threads
    if (m_packet_queue.size() > m_worker_threads.size() &&
        m_worker_threads.size() < m_max_threads)
    {
      m_worker_threads.emplace_back(&udp_server::worker_thread_proc, this);
      thread_added = true;
      thread_count = m_worker_threads.size();
    }
  }

  // Emit signals outside the lock to prevent re-entrancy deadlock
  if (thread_added)
  {
    if (onWorkerThreadCreated.slot_count() > 0) {
      onWorkerThreadCreated.emit(thread_count);
    }
    if (onActiveThreadsChanged.slot_count() > 0) {
      onActiveThreadsChanged.emit(thread_count);
    }
  }
}

/**
 * @brief Ensure the server is ready to start.
 *
 * @throws std::system_error If the server socket is invalid.
 * @throws std::runtime_error If required configuration is missing.
 */
void udp_server::validate_configuration() const
{
  if (!is_configured())
  {
    throw std::runtime_error("udp_server is not properly configured");
  }

  if (m_server_socket.is_closed())
  {
    throw std::logic_error("Server socket is closed or invalid");
  }

  if (!m_handler_factory)
  {
    throw std::runtime_error("Handler factory is not set");
  }
}

/**
 * @brief Join the receiver and worker threads, respecting a timeout.
 *
 * @param timeout Maximum duration allotted for thread shutdown.
 */
void udp_server::shutdown_threads(const std::chrono::milliseconds &timeout)
{
  (void)timeout; // Timeout used to signal urgency, but we always join to avoid use-after-free

  // Join receiver thread
  if (m_receiver_thread.joinable())
  {
    m_receiver_thread.join();
  }

  // Join worker threads - always join to prevent use-after-free from detached
  // threads accessing destroyed server object
  {
    std::lock_guard<std::mutex> lock(m_threads_mutex);
    for (auto &thread : m_worker_threads)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
    m_worker_threads.clear();
  }
}

/**
 * @brief Determine whether the server has the prerequisites to start.
 *
 * @return True if both socket and handler factory are configured.
 */
bool udp_server::is_configured() const { return m_has_socket && m_has_handler; }

/**
 * @brief Build a handler factory that always returns the shared handler.
 *
 * @param handler Shared handler instance.
 * @return Factory functor that ignores packet input and returns `handler`.
 */
udp_server::HandlerFactory
udp_server::create_shared_handler_factory(std::shared_ptr<udp_handler> handler)
{
  return
      [handler](const PacketData &packet_data) -> std::unique_ptr<udp_handler>
  {
    (void)packet_data; // Avoid unused parameter warning
    (void)handler;     // Avoid unused parameter warning
    // For shared handler, the factory is not used - packets are processed
    // directly
    return nullptr;
  };
}

} // namespace fb
