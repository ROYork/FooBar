#pragma once

/// @file connection.hpp
/// @brief HFT-optimized connection handles - minimal overhead design
///
/// Lightweight architecture:
/// - Direct shared_ptr to slot (no wrapper class)
/// - O(1) disconnect via atomic flag flip
/// - Minimal memory footprint

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "slot.hpp"

namespace fb {

// Forward declarations
template <typename... Args> class signal;

/// @brief Lightweight connection handle
///
/// Holds a type-erased shared_ptr to the slot.
/// Uses shared_ptr<void> with function pointers for type erasure.
class connection {
public:
  connection() noexcept = default;

  bool connected() const noexcept {
    return m_slot && m_is_active && m_is_active(m_slot.get());
  }

  void disconnect() noexcept {
    if (m_slot && m_deactivate) {
      m_deactivate(m_slot.get());
    }
  }

  bool valid() const noexcept { return m_slot != nullptr; }

  void block() noexcept {
    if (m_slot && m_block) {
      m_block(m_slot.get());
    }
  }

  void unblock() noexcept {
    if (m_slot && m_unblock) {
      m_unblock(m_slot.get());
    }
  }

  bool blocked() const noexcept {
    return m_slot && m_is_blocked && m_is_blocked(m_slot.get());
  }

  uint64_t id() const noexcept {
    return (m_slot && m_get_id) ? m_get_id(m_slot.get()) : 0;
  }

  bool operator==(const connection &other) const noexcept {
    return id() == other.id();
  }

  bool operator!=(const connection &other) const noexcept {
    return !(*this == other);
  }

private:
  template <typename... Args> friend class signal;

  template <typename... Args>
  explicit connection(std::shared_ptr<slot_entry<Args...>> slot)
      : m_slot(std::static_pointer_cast<void>(std::move(slot))) {
    using slot_t = slot_entry<Args...>;
    m_is_active = [](const void *p) -> bool {
      return static_cast<const slot_t *>(p)->is_active();
    };
    m_is_blocked = [](const void *p) -> bool {
      return static_cast<const slot_t *>(p)->is_blocked();
    };
    m_deactivate = [](void *p) { static_cast<slot_t *>(p)->deactivate(); };
    m_block = [](void *p) { static_cast<slot_t *>(p)->block(); };
    m_unblock = [](void *p) { static_cast<slot_t *>(p)->unblock(); };
    m_get_id = [](const void *p) -> uint64_t {
      return static_cast<const slot_t *>(p)->id();
    };
  }

  std::shared_ptr<void> m_slot;
  bool (*m_is_active)(const void *) = nullptr;
  bool (*m_is_blocked)(const void *) = nullptr;
  void (*m_deactivate)(void *) = nullptr;
  void (*m_block)(void *) = nullptr;
  void (*m_unblock)(void *) = nullptr;
  uint64_t (*m_get_id)(const void *) = nullptr;
};

/// @brief RAII connection wrapper
class scoped_connection {
public:
  scoped_connection() noexcept = default;
  scoped_connection(connection conn) noexcept : m_connection(std::move(conn)) {}
  ~scoped_connection() { disconnect(); }

  scoped_connection(scoped_connection &&other) noexcept
      : m_connection(std::move(other.m_connection)) {}

  scoped_connection &operator=(scoped_connection &&other) noexcept {
    if (this != &other) {
      disconnect();
      m_connection = std::move(other.m_connection);
    }
    return *this;
  }

  scoped_connection &operator=(connection conn) noexcept {
    disconnect();
    m_connection = std::move(conn);
    return *this;
  }

  scoped_connection(const scoped_connection &) = delete;
  scoped_connection &operator=(const scoped_connection &) = delete;

  void disconnect() noexcept { m_connection.disconnect(); }
  connection release() noexcept { return std::move(m_connection); }
  connection &get() noexcept { return m_connection; }
  const connection &get() const noexcept { return m_connection; }
  bool connected() const noexcept { return m_connection.connected(); }
  void block() noexcept { m_connection.block(); }
  void unblock() noexcept { m_connection.unblock(); }
  bool blocked() const noexcept { return m_connection.blocked(); }

private:
  connection m_connection;
};

/// @brief Container for multiple connections
class connection_guard {
public:
  connection_guard() = default;
  ~connection_guard() { disconnect_all(); }

  connection_guard(connection_guard &&) noexcept = default;
  connection_guard &operator=(connection_guard &&) noexcept = default;
  connection_guard(const connection_guard &) = delete;
  connection_guard &operator=(const connection_guard &) = delete;

  void add(connection conn) { m_connections.push_back(std::move(conn)); }

  connection_guard &operator+=(connection conn) {
    add(std::move(conn));
    return *this;
  }

  void disconnect_all() noexcept {
    for (auto &c : m_connections)
      c.disconnect();
    m_connections.clear();
  }

  void block_all() noexcept {
    for (auto &c : m_connections)
      c.block();
  }

  void unblock_all() noexcept {
    for (auto &c : m_connections)
      c.unblock();
  }

  std::size_t size() const noexcept { return m_connections.size(); }
  bool empty() const noexcept { return m_connections.empty(); }

  void cleanup() {
    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
                       [](const connection &c) { return !c.connected(); }),
        m_connections.end());
  }

private:
  std::vector<connection> m_connections;
};

/// @brief RAII blocker
class connection_blocker {
public:
  explicit connection_blocker(connection &conn) noexcept
      : m_connection(&conn), m_was_blocked(conn.blocked()) {
    if (!m_was_blocked)
      m_connection->block();
  }

  ~connection_blocker() {
    if (!m_was_blocked && m_connection)
      m_connection->unblock();
  }

  connection_blocker(const connection_blocker &) = delete;
  connection_blocker &operator=(const connection_blocker &) = delete;
  connection_blocker(connection_blocker &&) = delete;
  connection_blocker &operator=(connection_blocker &&) = delete;

private:
  connection *m_connection;
  bool m_was_blocked;
};

} // namespace fb
