#pragma once

#include <atomic>
#include <chrono>
#include <fb/socket_address.h>
#include <memory>

namespace fb
{

/**
 * @brief Base class for handling UDP packets in a multi-threaded server.
 *
 * This class serves as the foundation for udp_server packet handling.
 * 
 * Derived classes must implement the handle_packet() method to define
 * The udp_handler processes individual UDP packets
 * in a stateless manner and runs in worker threads.
 */
class udp_handler
{
public:

  udp_handler();

  /// @brief Copy/move operations disabled - handlers manage per-instance statistics
  udp_handler(const udp_handler &)            = delete;
  udp_handler(udp_handler &&)                 = delete;
  udp_handler & operator=(const udp_handler &) = delete;
  udp_handler & operator=(udp_handler &&)      = delete;

  virtual ~udp_handler()                                 = default;

  virtual void handle_packet(const void * buffer,
                             std::size_t length,
                             const socket_address & sender_address) = 0;

  bool process_packet(const void * buffer,
                      std::size_t length,
                      const socket_address & sender_address) noexcept;

  virtual std::string handler_name() const;
  virtual std::size_t max_packet_size() const;
  virtual bool can_handle_address(const socket_address & sender_address) const;

  std::uint64_t packets_processed() const;
  std::uint64_t bytes_processed() const;
  std::uint64_t error_count() const;

  std::chrono::steady_clock::time_point creation_time() const;
  std::chrono::steady_clock::time_point last_packet_time() const;

  void reset_statistics();

protected:

  virtual void handle_exception(const std::exception & ex,
                                const socket_address & sender_address) noexcept;

  virtual void on_packet_received(const void * buffer,
                                  std::size_t length,
                                  const socket_address & sender_address) noexcept;

  virtual void on_packet_processed(const void * buffer,
                                   std::size_t length,
                                   const socket_address & sender_address) noexcept;

  virtual bool validate_packet(const void * buffer,
                               std::size_t length,
                               const socket_address & sender_address) const;

private:
  // Statistics are atomic to support shared handlers called from multiple worker threads
  std::atomic<std::uint64_t> m_packets_processed{0};        ///< Number of packets processed
  std::atomic<std::uint64_t> m_bytes_processed{0};          ///< Number of bytes processed
  std::atomic<std::uint64_t> m_error_count{0};              ///< Number of errors encountered
  std::chrono::steady_clock::time_point m_creation_time;    ///< Handler creation time
  std::atomic<std::chrono::steady_clock::time_point::rep> m_last_packet_time_rep; ///< Last packet processing time (atomic rep)

  void initialize();
  void update_statistics(std::size_t length, bool success);
};

} // namespace fb
