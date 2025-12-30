/// @file string_builder.h
/// @brief Efficient string building with method chaining
///
/// The string_builder class provides high-performance string construction
/// with minimal memory allocations through pre-allocation and method chaining.
///
/// Features:
/// - Pre-allocation to avoid repeated reallocations
/// - Method chaining for fluent API
/// - Append operations for strings, characters, and numeric types
/// - Insert, remove, and replace operations
/// - Formatted string appending
/// - Stream-style operators for convenience
///
/// Thread Safety:
/// - string_builder is NOT thread-safe
/// - Create separate instances for multi-threaded use
///
/// Example:
/// @code
/// fb::string_builder sb;
/// sb.reserve(256);
/// sb.append("Hello, ")
///   .append("World!")
///   .append_line()
///   .append("Count: ")
///   .append_int(42);
/// std::string result = sb.to_string();
/// @endcode

#pragma once

#include "format.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace fb {

/**
 * @brief Efficient string builder with method chaining support
 *
 * Provides optimized string construction by managing a single buffer
 * that grows as needed. All append operations return a reference to
 * the builder for method chaining.
 */
class string_builder
{
public:

  string_builder() = default;
  explicit string_builder(std::size_t initial_capacity);
  explicit string_builder(std::string_view initial);

  ~string_builder() = default;

  // Non-copyable but moveable
  string_builder(const string_builder&)            = delete;
  string_builder& operator=(const string_builder&) = delete;
  string_builder(string_builder&&) noexcept        = default;
  string_builder& operator=(string_builder&&) noexcept = default;

  // ============================================================================
  //  Append Operations
  // ============================================================================

  string_builder& append(std::string_view str);
  string_builder& append(const char* str);
  string_builder& append(char c);
  string_builder& append(char c, std::size_t count);
  string_builder& append_line(std::string_view str = "");

  string_builder& append_int(std::int32_t value);
  string_builder& append_int(std::int64_t value);
  string_builder& append_uint(std::uint32_t value);
  string_builder& append_uint(std::uint64_t value);
  string_builder& append_double(double value, int precision = 6);
  string_builder& append_bool(bool value);


  // ============================================================================
  // Modification Operations
  // ============================================================================

  string_builder& insert(std::size_t pos, std::string_view str);
  string_builder& remove(std::size_t pos, std::size_t count = std::string::npos);
  string_builder& replace(std::string_view from, std::string_view to);
  string_builder& replace_all(std::string_view from, std::string_view to);

  // ============================================================================
  // Stream Operators
  // ============================================================================

  string_builder& operator<<(std::string_view str);
  string_builder& operator<<(char c);
  string_builder& operator<<(const char* str);
  string_builder& operator<<(std::int32_t value);
  string_builder& operator<<(std::int64_t value);
  string_builder& operator<<(std::uint32_t value);
  string_builder& operator<<(std::uint64_t value);
  string_builder& operator<<(double value);
  string_builder& operator<<(bool value);

  // ============================================================================
  // Output Methods
  // ============================================================================

  [[nodiscard]] std::string to_string() const;
  [[nodiscard]] std::string_view view() const noexcept;

  // ============================================================================
  // Management Methods
  // ============================================================================

  string_builder& clear() noexcept;
  void reserve(std::size_t capacity);
  [[nodiscard]] std::size_t size() const noexcept;
  [[nodiscard]] std::size_t capacity() const noexcept;
  [[nodiscard]] bool empty() const noexcept;



  /** @brief Append a formatted string
   *
   * Uses the fb::format system to format the string.
   *
   * @tparam Args Types of format arguments
   * @param fmt Format string
   * @param args Format arguments
   * @return Reference to this builder for chaining
   */
  template <typename... Args>
  string_builder& append_format(std::string_view fmt, Args&&... args)
  {
    m_buffer.append(format(fmt, std::forward<Args>(args)...));
    return *this;
  }


private:
  std::string m_buffer; ///< Internal buffer for string construction
};

} // namespace fb
