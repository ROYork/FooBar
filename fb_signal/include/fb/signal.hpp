#pragma once

/// @file signal.hpp
/// @brief High-performance signal class for type-safe event emission
///
/// The signal class provides:
/// - Type-safe slot connection with variadic arguments
/// - Lock-free emission (hot path)
/// - Thread-safe connection management (cold path)
/// - Priority-based slot ordering
/// - Optional filtering support
/// - Queued delivery for cross-thread communication

#include <algorithm>
#include <functional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include "connection.hpp"
#include "detail/slot_list.hpp"
#include "event_queue.hpp"
#include "slot.hpp"

namespace fb
{

/// @brief Filter predicate type for conditional slot invocation
template <typename... Args>
using slot_filter = std::function<bool(const Args &...)>;

/// @brief High-performance signal for decoupled component communication
///
/// A signal maintains a list of connected slots (callbacks) and invokes
/// them when emit() is called. The signal is parameterized by the argument
/// types that will be passed to slots.
///
/// **Performance characteristics:**
/// - Emission (hot path): Lock-free, zero allocation
/// - Connection (cold path): May allocate, uses mutex
/// - Disconnection: O(1) deactivation, deferred cleanup
///
/// **Thread safety:**
/// - Concurrent emissions from multiple threads: Safe
/// - Concurrent connect/disconnect: Safe
/// - Emission during connect/disconnect: Safe
///
/// Example:
/// @code
/// fb::signal<int, const std::string&> on_message;
///
/// auto conn = on_message.connect([](int id, const std::string& msg) {
///   std::cout << "Message " << id << ": " << msg << "\n";
/// });
///
/// on_message.emit(42, "Hello");  // Invokes the lambda
/// @endcode
template <typename... Args>
class signal
{
public:
  using slot_type = slot_entry<Args...>;
  using filter_type = slot_filter<Args...>;

  signal() = default;

  /// @brief Signals are non-copyable and non-movable
  ///
  /// The internal mutex-protected slot list prevents move semantics.
  /// Use pointers or references to share signals between components.
  signal(const signal &) = delete;
  signal &operator=(const signal &) = delete;
  signal(signal &&) = delete;
  signal &operator=(signal &&) = delete;

  /// @brief Destructor disconnects all slots
  ~signal()
  {
    disconnect_all();
  }

  // =========================================================================
  // Connection Methods
  // =========================================================================

  /// @brief Connect a callable as a slot with direct delivery
  ///
  /// The callable must be invocable with the signal's argument types.
  /// Supported callable types:
  /// - Lambda expressions
  /// - Free functions
  /// - Functor objects (classes with operator())
  /// - std::function wrappers
  ///
  /// @param func The callable to connect
  /// @param prio Priority level (higher = invoked earlier)
  /// @return Connection handle for managing the connection
  ///
  /// @note This is a cold-path operation that may allocate memory.
  template <typename F>
  connection connect(F &&func, priority prio = priority::normal)
  {
    static_assert(std::is_invocable_v<std::decay_t<F>, Args...>,
                  "Callable must be invocable with signal argument types");

    auto slot = m_slots.add(std::forward<F>(func), prio,
                            delivery_policy::direct, nullptr);
    return connection(slot);
  }

  /// @brief Connect a callable with a specific delivery policy
  ///
  /// @param func The callable to connect
  /// @param policy Delivery policy (direct, queued, or automatic)
  /// @param queue Target event queue for queued/automatic delivery
  /// @param prio Priority level (higher = invoked earlier)
  /// @return Connection handle for managing the connection
  ///
  /// @note For queued delivery, arguments must be copyable.
  /// @note For automatic delivery, direct is used if emitting thread
  ///       matches the queue's owner thread.
  template <typename F>
  connection connect(F &&func,
                     delivery_policy policy,
                     event_queue &queue,
                     priority prio = priority::normal)
  {
    static_assert(std::is_invocable_v<std::decay_t<F>, Args...>,
                  "Callable must be invocable with signal argument types");

    auto slot = m_slots.add(std::forward<F>(func), prio, policy, &queue);
    return connection(slot);
  }

  /// @brief Connect a member function as a slot
  ///
  /// Binds a member function to an object instance.
  ///
  /// @param obj Pointer to the object instance
  /// @param member_func Pointer to the member function
  /// @param prio Priority level
  /// @return Connection handle
  ///
  /// @warning The caller must ensure the object outlives the connection,
  /// or use scoped_connection tied to the object's lifetime.
  template <typename T, typename Ret>
  connection connect(T *obj, Ret (T::*member_func)(Args...),
                     priority prio = priority::normal)
  {
    return connect(
        [obj, member_func](Args... args)
        {
          (obj->*member_func)(std::forward<Args>(args)...);
        },
        prio);
  }

  /// @brief Connect a const member function as a slot
  template <typename T, typename Ret>
  connection connect(const T *obj, Ret (T::*member_func)(Args...) const,
                     priority prio = priority::normal)
  {
    return connect(
        [obj, member_func](Args... args)
        {
          (obj->*member_func)(std::forward<Args>(args)...);
        },
        prio);
  }

  /// @brief Connect a member function with delivery policy
  template <typename T, typename Ret>
  connection connect(T *obj, Ret (T::*member_func)(Args...),
                     delivery_policy policy,
                     event_queue &queue,
                     priority prio = priority::normal)
  {
    return connect(
        [obj, member_func](Args... args)
        {
          (obj->*member_func)(std::forward<Args>(args)...);
        },
        policy, queue, prio);
  }

  /// @brief Connect with a filter predicate
  ///
  /// The slot will only be invoked if the filter returns true.
  ///
  /// @param func The callable to connect
  /// @param filter Predicate that determines whether to invoke
  /// @param prio Priority level
  /// @return Connection handle
  ///
  /// @note Filtering adds overhead to each emission. Use sparingly.
  template <typename F>
  connection connect_filtered(F &&func, filter_type filter,
                              priority prio = priority::normal)
  {
    auto wrapper = [f = std::forward<F>(func),
                    filt = std::move(filter)](Args... args)
    {
      if (filt(args...))
      {
        f(std::forward<Args>(args)...);
      }
    };

    return connect(std::move(wrapper), prio);
  }

  // =========================================================================
  // Emission Methods
  // =========================================================================

  /// @brief Emit the signal, invoking all connected slots
  ///
  /// Invokes all active, non-blocked slots in priority order.
  /// This is the hot-path operation optimized for minimal latency:
  /// - Lock-free slot iteration
  /// - Zero heap allocation for direct delivery
  /// - Predictable execution
  ///
  /// For queued/automatic delivery, the slot invocation is posted to the
  /// target event_queue and executed when the owning thread processes it.
  ///
  /// @param args Arguments to forward to each slot
  ///
  /// **Reentrancy:** Slots may safely emit this or other signals.
  /// **Self-disconnect:** Slots may safely disconnect themselves.
  /// **Modification during emission:** New connections may or may not
  /// be invoked in the current emission (implementation-defined).
  void emit(Args... args) const
  {
    // Take atomic snapshot (single load, lock-free)
    auto snapshot = m_slots.get_snapshot();

    // Current thread ID for automatic delivery policy
    const auto current_thread = std::this_thread::get_id();

    // Invoke each active, non-blocked slot
    for (const auto &slot : *snapshot)
    {
      if (!slot->is_active() || slot->is_blocked())
      {
        continue;
      }

      const auto policy = slot->get_delivery_policy();
      auto *queue = slot->get_target_queue();

      bool use_direct = false;

      switch (policy)
      {
        case delivery_policy::direct:
          use_direct = true;
          break;

        case delivery_policy::queued:
          use_direct = (queue == nullptr);
          break;

        case delivery_policy::automatic:
          // Direct if same thread or no queue, otherwise queued
          if (queue == nullptr)
          {
            use_direct = true;
          }
          else
          {
            use_direct = (current_thread == queue->owner_thread());
          }
          break;
      }

      if (use_direct)
      {
        // For copyable value types: pass as lvalues so each slot receives
        // a copy. This prevents the first slot from moving-from the args
        // and leaving subsequent slots with moved-from (empty) data.
        //
        // For reference types or move-only types: must forward to preserve
        // reference semantics. Note that with multiple slots receiving
        // rvalue references, only the first slot receives valid data.
        // This is an inherent limitation of move-only/rvalue-ref broadcast.
        constexpr bool all_copyable_values =
            ((std::is_copy_constructible_v<std::decay_t<Args>> &&
              !std::is_reference_v<Args>) && ...);

        if constexpr (all_copyable_values)
        {
          slot->invoke_unchecked(args...);
        }
        else
        {
          slot->invoke_unchecked(std::forward<Args>(args)...);
        }
      }
      else if (queue != nullptr)
      {
        // Queue the invocation - capture args by value for cross-thread safety
        // Only enabled when all argument types are copyable
        if constexpr ((std::is_copy_constructible_v<std::decay_t<Args>> && ...))
        {
          enqueue_invocation(*queue, slot, args...);
        }
        else
        {
          // Fallback to direct delivery for non-copyable types
          slot->invoke_unchecked(std::forward<Args>(args)...);
        }
      }
    }
  }

  /// @brief Emit using function call syntax
  void operator()(Args... args) const
  {
    emit(args...);
  }

  // =========================================================================
  // Connection Management
  // =========================================================================

  /// @brief Disconnect all slots
  void disconnect_all()
  {
    m_slots.clear();
  }

  /// @brief Get the number of connected slots
  std::size_t slot_count() const noexcept
  {
    return m_slots.size();
  }

  /// @brief Check if any slots are connected
  bool empty() const noexcept
  {
    return m_slots.empty();
  }

  /// @brief Explicitly block a specific connection by ID
  void block(uint64_t connection_id)
  {
    if (auto slot = m_slots.find(connection_id))
    {
      slot->block();
    }
  }

  /// @brief Explicitly unblock a specific connection by ID
  void unblock(uint64_t connection_id)
  {
    if (auto slot = m_slots.find(connection_id))
    {
      slot->unblock();
    }
  }

  /// @brief Clean up deactivated slots
  ///
  /// This can be called periodically to reclaim memory from
  /// disconnected slots. Not required for correctness.
  void cleanup()
  {
    m_slots.cleanup();
  }

private:
  /// @brief Helper to enqueue a slot invocation for cross-thread delivery
  ///
  /// Captures arguments by value to ensure thread-safe access from the
  /// target thread. The slot is captured by shared_ptr to ensure it
  /// remains valid when the queued event is processed.
  template <typename... CapturedArgs>
  static void enqueue_invocation(event_queue &queue,
                                 std::shared_ptr<slot_type> slot,
                                 CapturedArgs... args)
  {
    // Capture slot and args by value for thread-safe delivery
    queue.enqueue([slot_copy = std::move(slot),
                   captured_args = std::make_tuple(args...)]() mutable
    {
      if (slot_copy->is_active() && !slot_copy->is_blocked())
      {
        std::apply([&slot_copy](auto &&...a)
        {
          slot_copy->invoke_unchecked(std::forward<decltype(a)>(a)...);
        }, std::move(captured_args));
      }
    });
  }

  detail::slot_list<Args...> m_slots;
};

/// @brief Type alias for signals with no arguments
using signal_void = signal<>;

} // namespace fb
