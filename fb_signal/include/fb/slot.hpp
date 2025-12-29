#pragma once

/// @file slot.hpp
/// @brief Type-erased callable wrapper with small-buffer optimization
///
/// Provides zero-allocation storage for small callables (lambdas with up to
/// ~6 captured pointers) while supporting arbitrary callable types.

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

#include "detail/function_traits.hpp"

namespace fb
{

/// @brief Slot priority levels (higher values = higher priority)
enum class priority : int32_t
{
  lowest = -1000,
  low = -100,
  normal = 0,
  high = 100,
  highest = 1000
};

/// @brief Delivery policy for slot invocation
enum class delivery_policy
{
  direct,   ///< Invoke immediately in emitting thread
  queued,   ///< Defer invocation to receiver's thread
  automatic ///< Direct if same thread, queued otherwise
};

// Forward declaration
class event_queue;

namespace detail
{

/// Small buffer size for callable storage (fits lambdas with ~6 captures)
constexpr std::size_t SBO_SIZE = 56;

/// @brief Type-erased callable storage with SBO
///
/// This class stores any callable matching the signature void(Args...)
/// using small-buffer optimization to avoid heap allocation for small types.
template <typename... Args>
class callable_storage
{
public:
  using invoke_fn = void (*)(void *, Args...);
  using destroy_fn = void (*)(void *);
  using move_fn = void (*)(void *, void *);

  callable_storage() noexcept = default;

  ~callable_storage()
  {
    destroy();
  }

  callable_storage(callable_storage &&other) noexcept
      : m_invoke(other.m_invoke)
      , m_destroy(other.m_destroy)
      , m_move(other.m_move)
      , m_is_heap(other.m_is_heap)
  {
    if (other.m_invoke)
    {
      if (m_is_heap)
      {
        // Transfer heap pointer
        std::memcpy(m_storage, other.m_storage, sizeof(void *));
        other.m_invoke = nullptr;
      }
      else
      {
        // Move from SBO storage
        m_move(m_storage, other.m_storage);
        other.destroy();
      }
    }
  }

  callable_storage &operator=(callable_storage &&other) noexcept
  {
    if (this != &other)
    {
      destroy();
      new (this) callable_storage(std::move(other));
    }
    return *this;
  }

  // Non-copyable
  callable_storage(const callable_storage &) = delete;
  callable_storage &operator=(const callable_storage &) = delete;

  /// @brief Store a callable
  template <typename F>
  void store(F &&func)
  {
    using DecayedF = std::decay_t<F>;
    static_assert(std::is_invocable_v<DecayedF, Args...>,
                  "Callable must be invocable with signal arguments");

    destroy();

    if constexpr (fits_in_sbo_v<DecayedF, SBO_SIZE>)
    {
      // Store in small buffer
      new (m_storage) DecayedF(std::forward<F>(func));
      m_is_heap = false;
    }
    else
    {
      // Heap allocate
      auto *ptr = new DecayedF(std::forward<F>(func));
      std::memcpy(m_storage, &ptr, sizeof(ptr));
      m_is_heap = true;
    }

    m_invoke = [](void *storage, Args... args)
    {
      DecayedF *callable;
      if constexpr (fits_in_sbo_v<DecayedF, SBO_SIZE>)
      {
        callable = reinterpret_cast<DecayedF *>(storage);
      }
      else
      {
        std::memcpy(&callable, storage, sizeof(callable));
      }
      (*callable)(std::forward<Args>(args)...);
    };

    m_destroy = [](void *storage)
    {
      DecayedF *callable;
      if constexpr (fits_in_sbo_v<DecayedF, SBO_SIZE>)
      {
        callable = reinterpret_cast<DecayedF *>(storage);
        callable->~DecayedF();
      }
      else
      {
        std::memcpy(&callable, storage, sizeof(callable));
        delete callable;
      }
    };

    m_move = [](void *dst, void *src)
    {
      if constexpr (fits_in_sbo_v<DecayedF, SBO_SIZE>)
      {
        auto *src_ptr = reinterpret_cast<DecayedF *>(src);
        new (dst) DecayedF(std::move(*src_ptr));
      }
      else
      {
        std::memcpy(dst, src, sizeof(void *));
      }
    };
  }

  /// @brief Invoke the stored callable
  void invoke(Args... args) const
  {
    if (m_invoke)
    {
      m_invoke(const_cast<void *>(static_cast<const void *>(m_storage)),
               std::forward<Args>(args)...);
    }
  }

  /// @brief Check if a callable is stored
  explicit operator bool() const noexcept
  {
    return m_invoke != nullptr;
  }

  /// @brief Check if callable is valid
  bool valid() const noexcept
  {
    return m_invoke != nullptr;
  }

private:
  void destroy()
  {
    if (m_destroy)
    {
      m_destroy(m_storage);
      m_invoke = nullptr;
      m_destroy = nullptr;
      m_move = nullptr;
    }
  }

  alignas(std::max_align_t) mutable char m_storage[SBO_SIZE]{};
  invoke_fn m_invoke = nullptr;
  destroy_fn m_destroy = nullptr;
  move_fn m_move = nullptr;
  bool m_is_heap = false;
};

} // namespace detail

/// @brief Slot entry containing callable and metadata
///
/// Each connected slot is represented by a slot_entry which holds:
/// - The type-erased callable
/// - Priority for ordering
/// - Active/blocked state flags
/// - Unique connection ID
/// - Delivery policy and target queue for cross-thread delivery
template <typename... Args>
class slot_entry
{
public:
  using id_type = uint64_t;

  slot_entry() = default;

  template <typename F>
  explicit slot_entry(F &&func,
                      priority prio = priority::normal,
                      delivery_policy policy = delivery_policy::direct,
                      event_queue *queue = nullptr)
      : m_priority(prio)
      , m_id(generate_id())
      , m_delivery_policy(policy)
      , m_target_queue(queue)
  {
    m_callable.store(std::forward<F>(func));
  }

  /// @brief Invoke the slot if active and not blocked
  void invoke(Args... args) const
  {
    if (is_active() && !is_blocked())
    {
      m_callable.invoke(std::forward<Args>(args)...);
    }
  }

  /// @brief Invoke unconditionally (for internal use)
  void invoke_unchecked(Args... args) const
  {
    m_callable.invoke(std::forward<Args>(args)...);
  }

  /// @brief Check if slot is active (not disconnected)
  bool is_active() const noexcept
  {
    return m_active.load(std::memory_order_acquire);
  }

  /// @brief Check if slot is temporarily blocked
  bool is_blocked() const noexcept
  {
    return m_blocked.load(std::memory_order_acquire);
  }

  /// @brief Mark slot as disconnected
  void deactivate() noexcept
  {
    m_active.store(false, std::memory_order_release);
  }

  /// @brief Block slot temporarily
  void block() noexcept
  {
    m_blocked.store(true, std::memory_order_release);
  }

  /// @brief Unblock slot
  void unblock() noexcept
  {
    m_blocked.store(false, std::memory_order_release);
  }

  /// @brief Get slot priority
  priority get_priority() const noexcept
  {
    return m_priority;
  }

  /// @brief Get unique connection ID
  id_type id() const noexcept
  {
    return m_id;
  }

  /// @brief Check if callable is valid
  bool valid() const noexcept
  {
    return m_callable.valid();
  }

  /// @brief Get delivery policy
  delivery_policy get_delivery_policy() const noexcept
  {
    return m_delivery_policy;
  }

  /// @brief Get target event queue (may be nullptr for direct delivery)
  event_queue *get_target_queue() const noexcept
  {
    return m_target_queue;
  }

private:
  static id_type generate_id() noexcept
  {
    static std::atomic<id_type> s_next_id{1};
    return s_next_id.fetch_add(1, std::memory_order_relaxed);
  }

  detail::callable_storage<Args...> m_callable;
  priority m_priority = priority::normal;
  id_type m_id = 0;
  std::atomic<bool> m_active{true};
  std::atomic<bool> m_blocked{false};
  delivery_policy m_delivery_policy = delivery_policy::direct;
  event_queue *m_target_queue = nullptr;
};

} // namespace fb
