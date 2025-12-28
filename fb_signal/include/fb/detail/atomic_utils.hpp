#pragma once

/// @file detail/atomic_utils.hpp
/// @brief Low-level atomic utilities for lock-free algorithms

#include <atomic>
#include <cstdint>
#include <thread>

namespace fb
{
namespace detail
{

/// Cache line size for padding to avoid false sharing
constexpr std::size_t CACHE_LINE_SIZE = 64;

/// @brief Align type to cache line boundary
template <typename T>
struct alignas(CACHE_LINE_SIZE) cache_aligned
{
  T value;

  cache_aligned() = default;

  explicit cache_aligned(T v)
      : value(v)
  {
  }

  operator T &()
  {
    return value;
  }

  operator const T &() const
  {
    return value;
  }
};

/// @brief Spin-wait with exponential backoff for busy loops
class spin_wait
{
public:
  void wait() noexcept
  {
    if (m_count < SPIN_THRESHOLD)
    {
      ++m_count;
      // CPU pause instruction for hyper-threading efficiency
#if defined(__x86_64__) || defined(_M_X64)
      __builtin_ia32_pause();
#elif defined(__aarch64__)
      asm volatile("yield");
#endif
    }
    else
    {
      // After spinning, yield to OS scheduler
      std::this_thread::yield();
    }
  }

  void reset() noexcept
  {
    m_count = 0;
  }

private:
  static constexpr uint32_t SPIN_THRESHOLD = 16;
  uint32_t m_count = 0;
};

/// @brief Epoch counter for deferred reclamation
/// Used to safely delete slots that may still be referenced by ongoing
/// emissions
class epoch_counter
{
public:
  using epoch_t = uint64_t;

  /// Get current global epoch
  epoch_t current() const noexcept
  {
    return m_global_epoch.load(std::memory_order_acquire);
  }

  /// Advance global epoch (called after connection changes)
  epoch_t advance() noexcept
  {
    return m_global_epoch.fetch_add(1, std::memory_order_acq_rel) + 1;
  }

  /// Enter critical section (emission start)
  epoch_t enter() noexcept
  {
    return m_global_epoch.load(std::memory_order_acquire);
  }

  /// Leave critical section (emission end)
  void leave(epoch_t) noexcept
  {
    // No-op in simplified implementation
    // Full implementation would track per-thread epochs
  }

private:
  std::atomic<epoch_t> m_global_epoch{0};
};

} // namespace detail
} // namespace fb
