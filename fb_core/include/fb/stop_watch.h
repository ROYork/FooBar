#pragma once

#include <chrono>
#include <mutex>

namespace fb {

/**
 * @brief High-resolution stopwatch for measuring elapsed time.
 *
 * The stopwatch uses std::chrono::steady_clock to provide nanosecond
 * resolution timing. All operations are thread-safe.
 */
class stop_watch
{
public:
  using clock_type = std::chrono::steady_clock;
  using duration = std::chrono::nanoseconds;

  explicit stop_watch(bool start_immediately = true) noexcept;
  void start() noexcept;
  void stop() noexcept;
  [[nodiscard]] duration elapsed_time() const noexcept;
  [[nodiscard]] bool is_running() const noexcept;
  void reset(bool start_immediately = true) noexcept;

private:

  mutable std::mutex m_mutex;
  clock_type::time_point m_start_time;
  duration m_accumulated;
  bool m_is_running;
};

// ============================================================================
// Implementation
// ============================================================================


/**
 * @brief Construct a new stop_watch
 *
 * Creates a high-resolution stopwatch for measuring elapsed time.
 * The stopwatch uses std::chrono::steady_clock for nanosecond precision
 * timing. All operations are thread-safe.
 *
 * @param start_immediately If true, the stopwatch starts immediately after construction.
 *                         If false, the stopwatch is created in stopped state.
 *
 * @note By default, the stopwatch starts automatically upon construction.
 *       Use start_immediately=false to create a stopped stopwatch.
 *
 * Example:
 * @code
 * // Stopwatch starts immediately (default)
 * fb::stop_watch sw1;
 *
 * // Stopwatch starts immediately
 * fb::stop_watch sw2(true);
 *
 * // Stopwatch is created but not running
 * fb::stop_watch sw3(false);
 * sw3.start();  // Manually start when needed
 * @endcode
 */
inline stop_watch::stop_watch(bool start_immediately) noexcept
  : m_start_time{}
  , m_accumulated(duration::zero())
  , m_is_running(false)
{
  if (start_immediately)
  {
    start();
  }
}

/**
 * @brief Start or resume the stopwatch
 *
 * If the stopwatch is not running, starts it from the current time.
 * If the stopwatch is already running, this method has no effect.
 *
 * Thread-safe and can be called from any thread.
 *
 * @note This method is safe to call multiple times. If already running,
 *       the call is ignored and no timing is affected.
 *
 * Example:
 * @code
 * fb::stop_watch sw;
 * sw.start();  // Start timing
 * // ... do some work ...
 * sw.start();  // No effect if already running
 * @endcode
 */
inline void stop_watch::start() noexcept
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_is_running)
  {
    return;
  }

  m_start_time = clock_type::now();
  m_is_running = true;
}

/**
 * @brief Stop the stopwatch
 *
 * Stops the stopwatch and accumulates the elapsed time. If the stopwatch
 * is not running, this method has no effect.
 *
 * Thread-safe and can be called from any thread.
 *
 * @note The accumulated time is preserved when stopping. Call reset()
 *       to clear accumulated time if needed.
 *
 * Example:
 * @code
 * fb::stop_watch sw;
 * sw.start();
 * // ... do some work ...
 * sw.stop();   // Stop timing and accumulate time
 * auto elapsed = sw.elapsed_time();
 * @endcode
 */
inline void stop_watch::stop() noexcept
{
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_is_running)
  {
    return;
  }

  const auto now = clock_type::now();
  m_accumulated += std::chrono::duration_cast<duration>(now - m_start_time);
  m_is_running = false;
}

/**
 * @brief Get the elapsed time
 *
 * Returns the total elapsed time since the stopwatch was created,
 * including any accumulated time from previous runs.
 *
 * @return Elapsed time in nanoseconds
 *
 * @note This method does not modify the stopwatch state. The stopwatch
 *       continues running if it was running before the call.
 *
 * @note The returned value is a snapshot and represents the elapsed time
 *       at the moment this method was called.
 *
 * Example:
 * @code
 * fb::stop_watch sw;
 * sw.start();
 * // ... do some work ...
 * auto elapsed = sw.elapsed_time();
 * std::cout << "Elapsed: " << elapsed.count() << "ns" << std::endl;
 *
 * // Can be called multiple times
 * auto elapsed2 = sw.elapsed_time();  // May be slightly larger
 * @endcode
 */
inline stop_watch::duration stop_watch::elapsed_time() const noexcept
{
  std::lock_guard<std::mutex> lock(m_mutex);
  auto total = m_accumulated;

  if (m_is_running)
  {
    const auto now = clock_type::now();
    total += std::chrono::duration_cast<duration>(now - m_start_time);
  }

  return total;
}

/**
 * @brief Check if the stopwatch is running
 *
 * Returns whether the stopwatch is currently timing.
 *
 * @return true if the stopwatch is running, false otherwise
 *
 * @note This method provides a snapshot of the current state. The state
 *       may change immediately after this call returns.
 *
 * Example:
 * @code
 * fb::stop_watch sw;
 * if (sw.is_running()) {
 *     std::cout << "Stopwatch is running" << std::endl;
 * } else {
 *     std::cout << "Stopwatch is stopped" << std::endl;
 * }
 * @endcode
 */
inline bool stop_watch::is_running() const noexcept
{
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_is_running;
}

/**
 * @brief Reset the stopwatch
 *
 * Resets the accumulated time to zero and optionally starts the stopwatch.
 * If start_immediately is true, the stopwatch starts running immediately.
 * If start_immediately is false, the stopwatch is left in stopped state.
 *
 * @param start_immediately If true, restart the stopwatch immediately after reset.
 *                         If false, leave the stopwatch in stopped state.
 *
 * @note This method clears all accumulated time and starts fresh timing
 *       if start_immediately is true.
 *
 * Example:
 * @code
 * fb::stop_watch sw;
 * sw.start();
 * // ... do some work ...
 * sw.reset();        // Clear time and restart
 *
 * fb::stop_watch sw2;
 * sw2.start();
 * // ... do some work ...
 * sw2.reset(false);  // Clear time but stay stopped
 * sw2.start();       // Start fresh timing
 * @endcode
 */
inline void stop_watch::reset(bool start_immediately) noexcept
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_accumulated = duration::zero();
  m_is_running = false;

  if (start_immediately)
  {
    m_start_time = clock_type::now();
    m_is_running = true;
  }
  else
  {
    m_start_time = clock_type::time_point{};
  }
}

}  // namespace fb

