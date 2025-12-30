/// @file string_builder.cpp
/// @brief Implementation of the string_builder class

#include "fb/string_builder.h"

#include <charconv>
#include <cstdio>
#include <limits>

namespace fb
{

// ============================================================================
// Constructors
// ============================================================================

/**
 * @brief Constructor with initial capacity
 * @param initial_capacity Number of bytes to pre-allocate
 */
string_builder::string_builder(std::size_t initial_capacity)
{
  m_buffer.reserve(initial_capacity);
}

/**
 * @brief Constructor with initial content
 * @param initial Initial string content
 */
string_builder::string_builder(std::string_view initial) :
  m_buffer(initial)
{
}

// ============================================================================
// String Append Operations
// ============================================================================


/**
 * @brief Append a string view
 * @param str String to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append(std::string_view str)
{
  m_buffer.append(str);
  return *this;
}


/**
 * @brief Append a C-string
 * @param str C-string to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append(const char* str)
{
  if (str != nullptr)
  {
    m_buffer.append(str);
  }
  return *this;
}


/**
 * @brief Append a single character
 * @param c Character to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append(char c)
{
  m_buffer.push_back(c);
  return *this;
}

/**
 * @brief Append a character repeated count times
 * @param c Character to append
 * @param count Number of times to repeat
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append(char c, std::size_t count)
{
  m_buffer.append(count, c);
  return *this;
}

/**
 * @brief Append a string followed by a newline
 * @param str String to append (default: empty string)
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_line(std::string_view str)
{
  m_buffer.append(str);
  m_buffer.push_back('\n');
  return *this;
}

// ============================================================================
// Numeric Append Operations
// ============================================================================


/**
 * @brief Append a 32-bit signed integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_int(std::int32_t value)
{
  // Use std::to_chars for efficient conversion
  char buffer[12]; // -2147483648 is 11 chars + null
  auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (ec == std::errc())
  {
    m_buffer.append(buffer, static_cast<std::size_t>(ptr - buffer));
  }
  return *this;
}

/**
 * @brief Append a 64-bit signed integer
 * @param value value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_int(std::int64_t value)
{
  char buffer[21]; // -9223372036854775808 is 20 chars + null
  auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (ec == std::errc())
  {
    m_buffer.append(buffer, static_cast<std::size_t>(ptr - buffer));
  }
  return *this;
}

/**
 * @brief Append a 32-bit unsigned integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_uint(std::uint32_t value)
{
  char buffer[11]; // 4294967295 is 10 chars + null
  auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (ec == std::errc())
  {
    m_buffer.append(buffer, static_cast<std::size_t>(ptr - buffer));
  }
  return *this;
}

/**
 * @brief Append a 64-bit unsigned integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_uint(std::uint64_t value)
{
  char buffer[21]; // 18446744073709551615 is 20 chars + null
  auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (ec == std::errc())
  {
    m_buffer.append(buffer, static_cast<std::size_t>(ptr - buffer));
  }
  return *this;
}

/**
 * @brief Append a double-precision floating point value
 * @param value Floating point value to append
 * @param precision Number of decimal places (default: 6)
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_double(double value, int precision)
{
  // Use snprintf for floating point formatting with precision control
  // Maximum double representation: -1.7976931348623157e+308 plus precision digits
  char buffer[64];
  int  len = std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
  if (len > 0 && static_cast<std::size_t>(len) < sizeof(buffer))
  {
    m_buffer.append(buffer, static_cast<std::size_t>(len));
  }
  return *this;
}

/**
 * @brief Append a boolean value as "true" or "false"
 * @param value Boolean value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::append_bool(bool value)
{
  m_buffer.append(value ? "true" : "false");
  return *this;
}

// ============================================================================
// Modification Operations
// ============================================================================

/**
 * @brief Insert a string at a position
 * @param pos Position to insert at (clamped to size if out of range)
 * @param str String to insert
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::insert(std::size_t pos, std::string_view str)
{
  if (pos > m_buffer.size())
  {
    pos = m_buffer.size();
  }
  m_buffer.insert(pos, str);
  return *this;
}

/**
 * @brief Remove characters from a position
 * @param pos Starting position
 * @param count Number of characters to remove (default: all remaining)
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::remove(std::size_t pos, std::size_t count)
{
  if (pos < m_buffer.size())
  {
    m_buffer.erase(pos, count);
  }
  return *this;
}

/**
 * @brief Replace first occurrence of a string
 * @param from String to find
 * @param to Replacement string
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::replace(std::string_view from, std::string_view to)
{
  if (from.empty())
  {
    return *this;
  }

  std::size_t pos = m_buffer.find(from);
  if (pos != std::string::npos)
  {
    m_buffer.replace(pos, from.size(), to);
  }
  return *this;
}

/**
 * @brief Replace all occurrences of a string
 * @param from String to find
 * @param to Replacement string
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::replace_all(std::string_view from, std::string_view to)
{
  if (from.empty())
  {
    return *this;
  }

  std::size_t pos = 0;
  while ((pos = m_buffer.find(from, pos)) != std::string::npos)
  {
    m_buffer.replace(pos, from.size(), to);
    pos += to.size();
  }
  return *this;
}

// ============================================================================
// Stream Operators
// ============================================================================

/**
 * @brief Stream operator for string view
 * @param str String to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(std::string_view str)
{
  return append(str);
}

/**
 * @brief Stream operator for character
 * @param c Character to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(char c)
{
  return append(c);
}

/**
 * @brief Stream operator for C-string
 * @param str C-string to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(const char* str)
{
  return append(str);
}

/**
 * @brief Stream operator for 32-bit signed integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(std::int32_t value)
{
  return append_int(value);
}

/**
 * @brief Stream operator for 64-bit signed integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(std::int64_t value)
{
  return append_int(value);
}

/**
 * @brief Stream operator for 32-bit unsigned integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(std::uint32_t value)
{
  return append_uint(value);
}

/**
 * @brief Stream operator for 64-bit unsigned integer
 * @param value Integer value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(std::uint64_t value)
{
  return append_uint(value);
}

/**
 * @brief Stream operator for double
 * @param value Double value to append
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(double value)
{
  return append_double(value);
}

/**
 * @brief Stream operator for boolean
 * @param value Boolean value to append as "true" or "false"
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::operator<<(bool value)
{
  return append_bool(value);
}

// ============================================================================
// Output Methods
// ============================================================================

/**
 * @brief Convert builder contents to string
 * @return String copy of the buffer contents
 */
std::string string_builder::to_string() const
{
  return m_buffer;
}

/**
 * @brief Get a view of the builder contents
 *
 * The view is invalidated by any subsequent append operations
 * or when the builder is destroyed.
 *
 * @return String view of the buffer contents
 */
std::string_view string_builder::view() const noexcept
{
  return m_buffer;
}

// ============================================================================
// Management Methods
// ============================================================================

/**
 * @brief Clear the builder contents
 *
 * Removes all content but preserves allocated capacity.
 *
 * @return Reference to this builder for chaining
 */
string_builder& string_builder::clear() noexcept
{
  m_buffer.clear();
  return *this;
}

/**
 * @brief Reserve capacity for future appends
 * @param capacity Minimum capacity to reserve
 */
void string_builder::reserve(std::size_t capacity)
{
  m_buffer.reserve(capacity);
}

/**
 * @brief Get current content size
 * @return Number of bytes in the buffer
 */
std::size_t string_builder::size() const noexcept
{
  return m_buffer.size();
}

/**
 * @brief Get current capacity
 * @return Number of bytes allocated
 */
std::size_t string_builder::capacity() const noexcept
{
  return m_buffer.capacity();
}

/**
 * @brief Check if builder is empty
 * @return true if the buffer contains no content
 */
bool string_builder::empty() const noexcept
{
  return m_buffer.empty();
}

} // namespace fb
