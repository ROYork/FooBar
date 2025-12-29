#pragma once

/// @file event_queue.hpp
/// @brief Lock-free MPSC event queue for cross-thread signal delivery
///
/// Provides a per-thread event processing mechanism for queued connections.
/// Multiple producer threads can enqueue events; a single consumer thread
/// processes them in FIFO order.
///
/// Uses small-buffer optimization to avoid heap allocation on the hot path.

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <thread>
#include <type_traits>

#include "detail/atomic_utils.hpp"

namespace fb
{

/// @brief Queue overflow policy
enum class overflow_policy
{
  drop_newest ///< Discard new events when queue is full (default, lock-free
              ///< safe)
};

namespace detail
{

/// @brief Size of inline storage for events (fits most lambdas with captures)
constexpr std::size_t EVENT_SBO_SIZE = 128;

/// @brief Fixed-size type-erased event with small-buffer optimization
///
/// Stores the callable inline if it fits, avoiding heap allocation.
/// This is critical for maintaining zero-allocation on the hot path.
class alignas(std::max_align_t) event
{
public:
  using invoke_fn  = void (*)(void *);
  using destroy_fn = void (*)(void *);
  using move_fn    = void (*)(void *, void *);

  event() noexcept = default;

  ~event()
  {
    destroy();
  }

  event(event &&other) noexcept :
    m_invoke(other.m_invoke),
    m_destroy(other.m_destroy),
    m_move(other.m_move)
  {
    if (other.m_invoke)
    {
      m_move(m_storage, other.m_storage);
      other.clear();
    }
  }

  event &operator=(event &&other) noexcept
  {
    if (this != &other)
    {
      destroy();
      m_invoke  = other.m_invoke;
      m_destroy = other.m_destroy;
      m_move    = other.m_move;
      if (other.m_invoke)
      {
        m_move(m_storage, other.m_storage);
        other.clear();
      }
    }
    return *this;
  }

  // Non-copyable
  event(const event &)            = delete;
  event &operator=(const event &) = delete;

  /// @brief Store a callable in the event
  ///
  /// Uses small-buffer optimization: if the callable fits in EVENT_SBO_SIZE
  /// bytes and is nothrow move constructible, it's stored inline.
  /// Otherwise, compilation fails (we don't fall back to heap allocation
  /// to maintain the zero-allocation guarantee).
  template <typename F>
  void store(F &&func)
  {
    using DecayedF = std::decay_t<F>;

    static_assert(sizeof(DecayedF) <= EVENT_SBO_SIZE,
                  "Callable too large for event SBO. Reduce capture size or "
                  "use smaller types. Maximum size is EVENT_SBO_SIZE bytes.");

    static_assert(alignof(DecayedF) <= alignof(std::max_align_t),
                  "Callable alignment exceeds max_align_t");

    static_assert(std::is_nothrow_move_constructible_v<DecayedF>,
                  "Callable must be nothrow move constructible for lock-free operation");

    destroy();

    new (m_storage) DecayedF(std::forward<F>(func));

    m_invoke = [](void *storage)
    {
      auto *callable = reinterpret_cast<DecayedF *>(storage);
      (*callable)();
    };

    m_destroy = [](void *storage)
    {
      auto *callable = reinterpret_cast<DecayedF *>(storage);
      callable->~DecayedF();
    };

    m_move = [](void *dst, void *src)
    {
      auto *src_ptr = reinterpret_cast<DecayedF *>(src);
      new (dst) DecayedF(std::move(*src_ptr));
      src_ptr->~DecayedF();
    };
  }

  /// @brief Invoke the stored callable
  void invoke()
  {
    if (m_invoke)
    {
      m_invoke(m_storage);
    }
  }

  /// @brief Check if event contains a callable
  bool valid() const noexcept
  {
    return m_invoke != nullptr;
  }

  explicit operator bool() const noexcept
  {
    return valid();
  }

private:
  void destroy()
  {
    if (m_destroy)
    {
      m_destroy(m_storage);
      clear();
    }
  }

  void clear() noexcept
  {
    m_invoke  = nullptr;
    m_destroy = nullptr;
    m_move    = nullptr;
  }

  alignas(std::max_align_t) char m_storage[EVENT_SBO_SIZE]{};
  invoke_fn m_invoke   = nullptr;
  destroy_fn m_destroy = nullptr;
  move_fn m_move       = nullptr;
};

/// @brief Lock-free MPSC ring buffer
///
/// Uses a power-of-two sized buffer with atomic head/tail indices.
/// Multiple producers can enqueue concurrently (lock-free CAS loop).
/// Single consumer dequeues sequentially.
template <typename T, std::size_t Capacity>
class ring_buffer
{
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of two");
  static_assert(Capacity > 0, "Capacity must be positive");

public:
  ring_buffer() = default;

  /// @brief Try to enqueue an item (lock-free, multi-producer safe)
  /// @return true if enqueued, false if queue was full
  bool try_enqueue(T &&item)
  {
    std::size_t tail = m_tail.load(std::memory_order_relaxed);

    for (;;)
    {
      std::size_t head      = m_head.load(std::memory_order_acquire);
      std::size_t next_tail = (tail + 1) & MASK;

      // Check if queue is full
      if (next_tail == head)
      {
        return false;
      }

      // Try to claim the slot
      if (m_tail.compare_exchange_weak(tail, next_tail,
                                       std::memory_order_acq_rel,
                                       std::memory_order_relaxed))
      {
        // Successfully claimed, store the item
        m_buffer[tail] = std::move(item);
        m_ready[tail].store(true, std::memory_order_release);
        return true;
      }
      // CAS failed, tail was updated, retry with new value
    }
  }

  /// @brief Try to dequeue an item (single consumer only)
  /// @param out_item Output parameter for the dequeued item
  /// @return true if dequeued, false if queue was empty
  bool try_dequeue(T &out_item)
  {
    std::size_t head = m_head.load(std::memory_order_relaxed);
    std::size_t tail = m_tail.load(std::memory_order_acquire);

    // Check if queue is empty
    if (head == tail)
    {
      return false;
    }

    // Wait for the slot to be ready (producer may still be writing)
    spin_wait waiter;
    while (!m_ready[head].load(std::memory_order_acquire))
    {
      waiter.wait();
    }

    // Read the item
    out_item = std::move(m_buffer[head]);
    m_ready[head].store(false, std::memory_order_release);

    // Advance head
    m_head.store((head + 1) & MASK, std::memory_order_release);

    return true;
  }

  /// @brief Get approximate queue size
  std::size_t size_approx() const noexcept
  {
    std::size_t head = m_head.load(std::memory_order_relaxed);
    std::size_t tail = m_tail.load(std::memory_order_relaxed);
    return (tail - head) & MASK;
  }

  /// @brief Check if queue is approximately empty
  bool empty_approx() const noexcept
  {
    return size_approx() == 0;
  }

  /// @brief Check if queue is approximately full
  bool full_approx() const noexcept
  {
    std::size_t head = m_head.load(std::memory_order_relaxed);
    std::size_t tail = m_tail.load(std::memory_order_relaxed);
    return ((tail + 1) & MASK) == head;
  }

private:
  static constexpr std::size_t MASK = Capacity - 1;

  alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> m_head{0};
  alignas(CACHE_LINE_SIZE) std::atomic<std::size_t> m_tail{0};

  std::array<T, Capacity> m_buffer{};
  std::array<std::atomic<bool>, Capacity> m_ready{};
};

} // namespace detail

/// @brief Per-thread event queue for deferred slot invocation
///
/// Each thread that needs to receive queued events should have its own
/// event_queue instance. Producer threads call enqueue(); the owning
/// thread calls process_pending() to invoke queued events.
///
/// **Zero-allocation guarantee:** Uses small-buffer optimization to store
/// events inline. Callables must fit within EVENT_SBO_SIZE (128 bytes)
/// to maintain the zero-allocation property on the hot path.
///
/// Default capacity: 4096 events (power-of-two for efficient indexing)
///
/// Example:
/// @code
/// thread_local fb::event_queue my_queue;
///
/// // Producer thread
/// my_queue.enqueue([msg = captured_msg]() {
///   process_message(msg);
/// });
///
/// // Consumer thread (owner)
/// while (running) {
///   my_queue.process_pending();
///   // ... other work ...
/// }
/// @endcode
class event_queue
{
public:
  /// Default queue capacity (4096 events)
  static constexpr std::size_t DEFAULT_CAPACITY = 4096;

  /// Maximum size for inline callable storage (bytes)
  static constexpr std::size_t MAX_CALLABLE_SIZE = detail::EVENT_SBO_SIZE;

  /// @brief Create an event queue for the current thread
  event_queue() :
    m_owner_thread(std::this_thread::get_id())
  {
  }

  /// @brief Enqueue an event for later processing (lock-free, zero-allocation)
  ///
  /// Can be called from any thread. The callable will be invoked later
  /// when the owning thread calls process_pending().
  ///
  /// @param func Callable to invoke (must fit in MAX_CALLABLE_SIZE bytes)
  /// @return true if enqueued, false if queue was full
  ///
  /// @note This operation performs NO heap allocation. The callable is
  /// stored inline using small-buffer optimization.
  template <typename F>
  bool enqueue(F &&func)
  {
    detail::event evt;
    evt.store(std::forward<F>(func));

    bool success = m_queue.try_enqueue(std::move(evt));
    if (!success)
    {
      m_dropped_count.fetch_add(1, std::memory_order_relaxed);
    }
    return success;
  }

  /// @brief Process all pending events (single-threaded, called by owner)
  ///
  /// Invokes all queued events in FIFO order. Should only be called
  /// from the thread that owns this queue.
  ///
  /// @return Number of events processed
  std::size_t process_pending()
  {
    std::size_t count = 0;
    detail::event evt;

    while (m_queue.try_dequeue(evt))
    {
      if (evt)
      {
        evt.invoke();
      }
      ++count;
    }

    return count;
  }

  /// @brief Process up to max_events pending events
  /// @param max_events Maximum number of events to process
  /// @return Number of events actually processed
  std::size_t process_pending(std::size_t max_events)
  {
    std::size_t count = 0;
    detail::event evt;

    while (count < max_events && m_queue.try_dequeue(evt))
    {
      if (evt)
      {
        evt.invoke();
      }
      ++count;
    }

    return count;
  }

  /// @brief Check if this thread is the queue owner
  bool is_owner_thread() const noexcept
  {
    return std::this_thread::get_id() == m_owner_thread;
  }

  /// @brief Get approximate pending event count
  std::size_t pending_count() const noexcept
  {
    return m_queue.size_approx();
  }

  /// @brief Check if queue is approximately empty
  bool empty() const noexcept
  {
    return m_queue.empty_approx();
  }

  /// @brief Get number of dropped events due to overflow
  uint64_t dropped_count() const noexcept
  {
    return m_dropped_count.load(std::memory_order_relaxed);
  }

  /// @brief Get the owner thread ID
  std::thread::id owner_thread() const noexcept
  {
    return m_owner_thread;
  }

private:
  detail::ring_buffer<detail::event, DEFAULT_CAPACITY> m_queue;
  std::thread::id m_owner_thread;
  std::atomic<uint64_t> m_dropped_count{0};
};

/// @brief Get the thread-local event queue for the current thread
///
/// Each thread has its own event queue for receiving queued events.
/// This function provides access to it.
inline event_queue &thread_event_queue()
{
  thread_local event_queue queue;
  return queue;
}

} // namespace fb
