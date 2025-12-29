#pragma once

/// @file detail/slot_list.hpp
/// @brief HFT-optimized slot container with bounded memory growth
///
/// Uses copy-on-write for thread safety with O(1) disconnect.
/// Key properties:
/// - Connect: Filters inactive slots (automatic cleanup)
/// - Disconnect: O(1) atomic flag flip
/// - Memory: Auto-cleanup when inactive ratio exceeds threshold

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "../slot.hpp"

namespace fb {
namespace detail {

/// @brief Thread-safe slot container optimized for HFT workloads
template <typename... Args> class slot_list {
public:
  using slot_type = slot_entry<Args...>;
  using slot_ptr = std::shared_ptr<slot_type>;
  using container_type = std::vector<slot_ptr>;
  using snapshot_type = std::shared_ptr<const container_type>;

  /// Maximum ratio of inactive to total slots before cleanup triggers
  static constexpr double CLEANUP_THRESHOLD = 0.5; // 50% dead triggers cleanup

  slot_list()
      : m_slots(std::make_shared<container_type>()), m_inactive_count(0) {}

  /// @brief Add a new slot
  template <typename F>
  slot_ptr add(F &&func, priority prio = priority::normal,
               delivery_policy policy = delivery_policy::direct,
               event_queue *queue = nullptr) {
    auto slot =
        std::make_shared<slot_type>(std::forward<F>(func), prio, policy, queue);

    std::lock_guard<std::mutex> lock(m_mutex);

    auto current = get_snapshot_internal();

    // Create new container filtering inactive slots (cleanup during add)
    auto new_slots = std::make_shared<container_type>();
    new_slots->reserve(current->size() + 1);

    for (const auto &existing : *current) {
      if (existing && existing->is_active()) {
        new_slots->push_back(existing);
      }
    }

    new_slots->push_back(slot);

    // Sort by priority (higher first), then by ID (earlier first)
    std::stable_sort(new_slots->begin(), new_slots->end(),
                     [](const slot_ptr &a, const slot_ptr &b) {
                       const int32_t pa =
                           static_cast<int32_t>(a->get_priority());
                       const int32_t pb =
                           static_cast<int32_t>(b->get_priority());
                       if (pa != pb) {
                         return pa > pb;
                       }
                       return a->id() < b->id();
                     });

    std::atomic_store_explicit(&m_slots, new_slots, std::memory_order_release);
    m_inactive_count.store(0, std::memory_order_relaxed); // Reset after cleanup
    return slot;
  }

  /// @brief Deactivate a slot by ID - O(1)
  /// Triggers cleanup if inactive ratio exceeds threshold
  bool remove(typename slot_type::id_type id) {
    auto snapshot = get_snapshot();
    for (const auto &slot : *snapshot) {
      if (slot && slot->id() == id) {
        slot->deactivate(); // O(1) atomic operation

        // Track inactive count for threshold-based cleanup
        const auto inactive =
            m_inactive_count.fetch_add(1, std::memory_order_relaxed) + 1;
        const auto total = snapshot->size();

        // Auto-cleanup if inactive ratio exceeds threshold
        if (total > 0 &&
            static_cast<double>(inactive) / total > CLEANUP_THRESHOLD) {
          cleanup_internal();
        }
        return true;
      }
    }
    return false;
  }

  /// @brief Remove all slots
  void clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto current = get_snapshot_internal();
    for (const auto &slot : *current) {
      if (slot) {
        slot->deactivate();
      }
    }

    std::atomic_store_explicit(&m_slots, std::make_shared<container_type>(),
                               std::memory_order_release);
    m_inactive_count.store(0, std::memory_order_relaxed);
  }

  /// @brief Get atomic snapshot (lock-free)
  snapshot_type get_snapshot() const noexcept {
    return std::atomic_load_explicit(&m_slots, std::memory_order_acquire);
  }

  std::size_t size() const noexcept {
    auto snapshot = get_snapshot();
    std::size_t count = 0;
    for (const auto &slot : *snapshot) {
      if (slot && slot->is_active()) {
        ++count;
      }
    }
    return count;
  }

  bool empty() const noexcept {
    auto snapshot = get_snapshot();
    for (const auto &slot : *snapshot) {
      if (slot && slot->is_active()) {
        return false;
      }
    }
    return true;
  }

  slot_ptr find(typename slot_type::id_type id) const {
    auto snapshot = get_snapshot();
    for (const auto &slot : *snapshot) {
      if (slot && slot->id() == id && slot->is_active()) {
        return slot;
      }
    }
    return nullptr;
  }

  /// @brief Manual cleanup (also called automatically when threshold exceeded)
  void cleanup() { cleanup_internal(); }

private:
  void cleanup_internal() {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto current = get_snapshot_internal();

    // Count active slots
    std::size_t active_count = 0;
    for (const auto &slot : *current) {
      if (slot && slot->is_active()) {
        ++active_count;
      }
    }

    // Skip if all active
    if (active_count == current->size()) {
      m_inactive_count.store(0, std::memory_order_relaxed);
      return;
    }

    auto new_slots = std::make_shared<container_type>();
    new_slots->reserve(active_count);

    for (const auto &slot : *current) {
      if (slot && slot->is_active()) {
        new_slots->push_back(slot);
      }
    }

    std::atomic_store_explicit(&m_slots, new_slots, std::memory_order_release);
    m_inactive_count.store(0, std::memory_order_relaxed);
  }

  snapshot_type get_snapshot_internal() const {
    return std::atomic_load_explicit(&m_slots, std::memory_order_acquire);
  }

  std::shared_ptr<container_type> m_slots;
  mutable std::mutex m_mutex;
  std::atomic<std::size_t> m_inactive_count;
};

} // namespace detail
} // namespace fb
