#include <fb/udp_handler.h>
#include <chrono>
#include <stdexcept>
#include <typeinfo>

namespace fb
{

/**
 * @class fb::udp_handler
 * @brief Base class for processing individual UDP datagrams.
 *
 * Custom handlers derive from this class, override `handle_packet()`, and
 * optionally customize validation, hooks, and error handling. The base class
 * tracks statistics and provides a safe wrapper through `process_packet()`.
 */

/**
 * @brief Construct a handler and initialise statistics.
 */
udp_handler::udp_handler() :
  m_packets_processed(0),
  m_bytes_processed(0),
  m_error_count(0),
  m_creation_time(std::chrono::steady_clock::now()),
  m_last_packet_time_rep(m_creation_time.time_since_epoch().count())
{
}

/**
 * @brief Process a packet via the handler pipeline.
 *
 * @param buffer Packet payload.
 * @param length Payload length in bytes.
 * @param sender_address Source endpoint of the packet.
 * @return True if the packet was accepted and processed successfully.
 *
 * The wrapper validates parameters, enforces address and size limits, invokes
 * the validation and hook callbacks, and records statistics. Exceptions are
 * caught and forwarded to `handle_exception()`.
 */
bool udp_handler::process_packet(const void *buffer,
                                 std::size_t length,
                                 const socket_address &sender_address) noexcept
{
  try
  {
    // Validate input parameters
    if (!buffer || length == 0)
    {
      update_statistics(length, false);
      return false;
    }

    // Check if handler can process packets from this address
    if (!can_handle_address(sender_address))
    {
      update_statistics(length, false);
      return false;
    }

    // Check packet size limits
    std::size_t max_size = max_packet_size();
    if (max_size > 0 && length > max_size)
    {
      update_statistics(length, false);
      return false;
    }

    // Validate packet data
    if (!validate_packet(buffer, length, sender_address))
    {
      update_statistics(length, false);
      return false;
    }

    // Pre-processing hook
    on_packet_received(buffer, length, sender_address);

    // Process the packet
    handle_packet(buffer, length, sender_address);

    // Post-processing hook
    on_packet_processed(buffer, length, sender_address);

    // Update statistics for successful processing
    update_statistics(length, true);
    return true;
  }
  catch (const std::exception &ex)
  {
    handle_exception(ex, sender_address);
    update_statistics(length, false);
    return false;
  }
  catch (...)
  {
    // Handle unknown exceptions
    try
    {
      std::runtime_error unknown_ex(
          "Unknown exception in udp_handler::process_packet");
      handle_exception(unknown_ex, sender_address);
    }
    catch (...)
    {
      // Ignore exceptions in exception handler
    }
    update_statistics(length, false);
    return false;
  }
}

/**
 * @brief Return a human-readable handler name.
 *
 * @return RTTI-based class name by default.
 */
std::string udp_handler::handler_name() const
{
  // Return the class name using RTTI
  return typeid(*this).name();
}

/**
 * @brief Report the maximum packet size accepted by this handler.
 *
 * @return Maximum size in bytes, or zero when unlimited.
 */
std::size_t udp_handler::max_packet_size() const
{
  // Default: no limit
  return 0;
}

/**
 * @brief Determine if the handler will accept packets from the sender.
 *
 * @param sender_address Source endpoint to evaluate.
 * @return True by default; override to apply filtering.
 */
bool udp_handler::can_handle_address(const socket_address &sender_address) const
{
  // Default: accept all addresses
  (void)sender_address; // Avoid unused parameter warning
  return true;
}

/**
 * @brief Total number of successfully processed packets.
 */
std::uint64_t udp_handler::packets_processed() const
{
  return m_packets_processed.load(std::memory_order_relaxed);
}

/**
 * @brief Total number of bytes processed by the handler.
 */
std::uint64_t udp_handler::bytes_processed() const
{
  return m_bytes_processed.load(std::memory_order_relaxed);
}

/**
 * @brief Total number of packets that resulted in errors.
 */
std::uint64_t udp_handler::error_count() const
{
  return m_error_count.load(std::memory_order_relaxed);
}

/**
 * @brief Timestamp noting when the handler was created.
 */
std::chrono::steady_clock::time_point udp_handler::creation_time() const
{
  return m_creation_time;
}

/**
 * @brief Timestamp of the most recent packet handled.
 */
std::chrono::steady_clock::time_point udp_handler::last_packet_time() const
{
  auto rep = m_last_packet_time_rep.load(std::memory_order_relaxed);
  return std::chrono::steady_clock::time_point(
      std::chrono::steady_clock::duration(rep));
}

/**
 * @brief Reset tracked statistics back to zero.
 */
void udp_handler::reset_statistics()
{
  m_packets_processed.store(0, std::memory_order_relaxed);
  m_bytes_processed.store(0, std::memory_order_relaxed);
  m_error_count.store(0, std::memory_order_relaxed);
  m_creation_time = std::chrono::steady_clock::now();
  m_last_packet_time_rep.store(m_creation_time.time_since_epoch().count(),
                               std::memory_order_relaxed);
}

/**
 * @brief Default exception handler invoked when processing fails.
 *
 * @param ex Exception instance.
 * @param sender_address Source endpoint that triggered the exception.
 *
 * The base implementation ignores the exception; override in derived classes
 * to log or react differently.
 */
void udp_handler::handle_exception(const std::exception & ex,
                                   const socket_address & sender_address) noexcept
{
  // Default implementation: could log the exception in a real implementation
  // For now, we just continue processing
  static_cast<void>(ex);             // Avoid unused parameter warning
  static_cast<void>(sender_address); // Avoid unused parameter warning

  // In a production system, you might want to log this:
  // LOG_ERROR("udp_handler exception from " << sender_address.to_string() << ":
  // " << ex.what());
}

/**
 * @brief Hook executed before `handle_packet()` runs.
 *
 * Parameters mirror those passed to `handle_packet()`. The default
 * implementation does nothing.
 */
void udp_handler::on_packet_received(const void * buffer,
                                     std::size_t length,
                                     const socket_address & sender_address) noexcept
{
  // Default implementation does nothing
  static_cast<void>(buffer);         // Avoid unused parameter warning
  static_cast<void>(length);         // Avoid unused parameter warning
  static_cast<void>(sender_address); // Avoid unused parameter warning
}

/**
 * @brief Hook executed after a packet has been processed successfully.
 *
 * Parameters mirror those passed to `handle_packet()`. The default
 * implementation does nothing.
 */
void udp_handler::on_packet_processed(
    const void *buffer,
    std::size_t length,
    const socket_address &sender_address) noexcept
{
  // Default implementation does nothing
  static_cast<void>(buffer);         // Avoid unused parameter warning
  static_cast<void>(length);         // Avoid unused parameter warning
  static_cast<void>(sender_address); // Avoid unused parameter warning
}

/**
 * @brief Validate a packet before handing it to `handle_packet()`.
 *
 * @param buffer Packet payload.
 * @param length Payload length.
 * @param sender_address Source endpoint.
 * @return True when the packet should be processed; false to drop.
 *
 * The base implementation accepts every packet.
 */
bool udp_handler::validate_packet(const void *buffer,
                                  std::size_t length,
                                  const socket_address &sender_address) const
{
  // Default implementation accepts all packets
  static_cast<void>(buffer);         // Avoid unused parameter warning
  static_cast<void>(length);         // Avoid unused parameter warning
  static_cast<void>(sender_address); // Avoid unused parameter warning
  return true;
}

/**
 * @brief Update statistics after processing completes.
 *
 * @param length Packet size.
 * @param success True if the packet was handled successfully.
 */
void udp_handler::update_statistics(std::size_t length, bool success)
{
  if (success)
  {
    m_packets_processed.fetch_add(1, std::memory_order_relaxed);
    m_bytes_processed.fetch_add(length, std::memory_order_relaxed);
  }
  else
  {
    m_error_count.fetch_add(1, std::memory_order_relaxed);
  }

  auto now = std::chrono::steady_clock::now();
  m_last_packet_time_rep.store(now.time_since_epoch().count(),
                               std::memory_order_relaxed);
}

} // namespace fb
