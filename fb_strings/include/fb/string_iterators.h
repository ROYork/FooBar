/// @file string_iterators.h
/// @brief Zero-copy string iterators for efficient parsing
///
/// Provides iterator classes for parsing strings without creating
/// intermediate string copies. All iterators work with string_view
/// and return string_view for zero-copy access.
///
/// Available iterators:
/// - line_iterator: Iterate over lines (handles \r, \n, \r\n)
/// - word_iterator: Iterate over whitespace-separated words
/// - token_iterator: Iterate over tokens with custom delimiters
/// - codepoint_iterator: Iterate over UTF-8 code points
///
/// Thread Safety:
/// - Iterators are NOT thread-safe
/// - The source string must remain valid during iteration
///
/// Example:
/// @code
/// std::string text = "Hello World\nGoodbye";
///
/// // Line iteration
/// fb::line_iterator lines(text);
/// while (lines.has_next()) {
///     std::string_view line = lines.next();
///     // process line...
/// }
///
/// // Word iteration
/// for (fb::word_iterator words(text); words.has_next(); ) {
///     std::string_view word = words.next();
///     // process word...
/// }
/// @endcode

#pragma once

#include <cstddef>
#include <string_view>

namespace fb
{

// ============================================================================
// line_iterator
// ============================================================================

/// @brief Zero-copy iterator over lines in a string
///
/// Handles multiple line ending styles: \r, \n, and \r\n.
/// Empty lines are returned as empty string_view.
class line_iterator
{
public:
  /// @brief Construct line iterator
  /// @param str String to iterate over (must remain valid during iteration)
  explicit line_iterator(std::string_view str) noexcept
      : m_str(str)
      , m_pos(0)
      , m_original(str)
  {
  }

  /// @brief Check if more lines are available
  /// @return true if there are more lines to read
  [[nodiscard]] bool has_next() const noexcept
  {
    return m_pos <= m_str.size();
  }

  /// @brief Get the next line
  ///
  /// @return Next line as string_view (without line ending)
  /// @pre has_next() must be true
  std::string_view next() noexcept
  {
    if (m_pos > m_str.size())
    {
      return {};
    }

    std::size_t start = m_pos;
    std::size_t end   = m_pos;

    // Find line ending
    while (end < m_str.size())
    {
      char c = m_str[end];
      if (c == '\n')
      {
        m_pos = end + 1;
        return m_str.substr(start, end - start);
      }
      if (c == '\r')
      {
        m_pos = end + 1;
        // Check for \r\n
        if (m_pos < m_str.size() && m_str[m_pos] == '\n')
        {
          ++m_pos;
        }
        return m_str.substr(start, end - start);
      }
      ++end;
    }

    // Last line (no trailing newline)
    m_pos = m_str.size() + 1; // Signal end
    return m_str.substr(start);
  }

  /// @brief Reset iterator to beginning
  void reset() noexcept
  {
    m_str = m_original;
    m_pos = 0;
  }

private:
  std::string_view m_str;      ///< Current view
  std::size_t      m_pos;      ///< Current position
  std::string_view m_original; ///< Original string for reset
};

// ============================================================================
// word_iterator
// ============================================================================

/// @brief Zero-copy iterator over whitespace-separated words
///
/// Splits on any whitespace character (space, tab, newline, etc.).
/// Consecutive whitespace is treated as a single separator.
class word_iterator
{
public:
  /// @brief Construct word iterator
  /// @param str String to iterate over (must remain valid during iteration)
  explicit word_iterator(std::string_view str) noexcept
      : m_str(str)
      , m_pos(0)
      , m_original(str)
  {
    skip_whitespace();
  }

  /// @brief Check if more words are available
  /// @return true if there are more words to read
  [[nodiscard]] bool has_next() const noexcept
  {
    return m_pos < m_str.size();
  }

  /// @brief Get the next word
  ///
  /// @return Next word as string_view
  /// @pre has_next() must be true
  std::string_view next() noexcept
  {
    if (m_pos >= m_str.size())
    {
      return {};
    }

    std::size_t start = m_pos;

    // Find end of word
    while (m_pos < m_str.size() && !is_whitespace(m_str[m_pos]))
    {
      ++m_pos;
    }

    std::string_view result = m_str.substr(start, m_pos - start);

    // Skip trailing whitespace for next call
    skip_whitespace();

    return result;
  }

  /// @brief Reset iterator to beginning
  void reset() noexcept
  {
    m_str = m_original;
    m_pos = 0;
    skip_whitespace();
  }

private:
  void skip_whitespace() noexcept
  {
    while (m_pos < m_str.size() && is_whitespace(m_str[m_pos]))
    {
      ++m_pos;
    }
  }

  static constexpr bool is_whitespace(char c) noexcept
  {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
  }

  std::string_view m_str;      ///< Current view
  std::size_t      m_pos;      ///< Current position
  std::string_view m_original; ///< Original string for reset
};

// ============================================================================
// token_iterator
// ============================================================================

/// @brief Zero-copy iterator over tokens with custom delimiters
///
/// Splits on any character from the delimiter set.
/// Empty tokens between consecutive delimiters are returned.
class token_iterator
{
public:
  /// @brief Construct token iterator
  /// @param str String to iterate over (must remain valid during iteration)
  /// @param delimiters Characters to split on (must remain valid during iteration)
  token_iterator(std::string_view str, std::string_view delimiters) noexcept
      : m_str(str)
      , m_delimiters(delimiters)
      , m_pos(0)
      , m_has_more(true)
      , m_original(str)
  {
    if (m_str.empty())
    {
      m_has_more = false;
    }
  }

  /// @brief Check if more tokens are available
  /// @return true if there are more tokens to read
  [[nodiscard]] bool has_next() const noexcept
  {
    return m_has_more;
  }

  /// @brief Get the next token
  ///
  /// @return Next token as string_view (may be empty)
  /// @pre has_next() must be true
  std::string_view next() noexcept
  {
    if (!m_has_more)
    {
      return {};
    }

    std::size_t start = m_pos;

    // Find delimiter
    while (m_pos < m_str.size() && !is_delimiter(m_str[m_pos]))
    {
      ++m_pos;
    }

    std::string_view result = m_str.substr(start, m_pos - start);

    if (m_pos < m_str.size())
    {
      ++m_pos; // Skip delimiter
    }
    else
    {
      m_has_more = false;
    }

    return result;
  }

  /// @brief Reset iterator to beginning
  void reset() noexcept
  {
    m_str      = m_original;
    m_pos      = 0;
    m_has_more = !m_str.empty();
  }

private:
  bool is_delimiter(char c) const noexcept
  {
    return m_delimiters.find(c) != std::string_view::npos;
  }

  std::string_view m_str;        ///< Current view
  std::string_view m_delimiters; ///< Delimiter characters
  std::size_t      m_pos;        ///< Current position
  bool             m_has_more;   ///< Has more tokens
  std::string_view m_original;   ///< Original string for reset
};

// ============================================================================
// codepoint_iterator
// ============================================================================

/// @brief Zero-copy iterator over UTF-8 code points
///
/// Returns each UTF-8 code point as a string_view (1-4 bytes).
/// Invalid UTF-8 sequences are returned as single bytes.
class codepoint_iterator
{
public:
  /// @brief Construct code point iterator
  /// @param str UTF-8 string to iterate over (must remain valid during iteration)
  explicit codepoint_iterator(std::string_view str) noexcept
      : m_str(str)
      , m_pos(0)
      , m_original(str)
  {
  }

  /// @brief Check if more code points are available
  /// @return true if there are more code points to read
  [[nodiscard]] bool has_next() const noexcept
  {
    return m_pos < m_str.size();
  }

  /// @brief Get the next code point
  ///
  /// @return Next code point as string_view (1-4 bytes)
  /// @pre has_next() must be true
  std::string_view next() noexcept
  {
    if (m_pos >= m_str.size())
    {
      return {};
    }

    std::size_t start  = m_pos;
    std::size_t cp_len = codepoint_length(static_cast<unsigned char>(m_str[m_pos]));

    // Validate continuation bytes
    if (cp_len > 1 && start + cp_len <= m_str.size())
    {
      for (std::size_t i = 1; i < cp_len; ++i)
      {
        if (!is_continuation_byte(static_cast<unsigned char>(m_str[start + i])))
        {
          // Invalid sequence - return single byte
          cp_len = 1;
          break;
        }
      }
    }
    else if (start + cp_len > m_str.size())
    {
      // Truncated sequence - return single byte
      cp_len = 1;
    }

    m_pos = start + cp_len;
    return m_str.substr(start, cp_len);
  }

  /// @brief Reset iterator to beginning
  void reset() noexcept
  {
    m_str = m_original;
    m_pos = 0;
  }

private:
  static constexpr std::size_t codepoint_length(unsigned char first_byte) noexcept
  {
    if ((first_byte & 0x80) == 0x00)
    {
      return 1; // ASCII
    }
    if ((first_byte & 0xE0) == 0xC0)
    {
      return 2;
    }
    if ((first_byte & 0xF0) == 0xE0)
    {
      return 3;
    }
    if ((first_byte & 0xF8) == 0xF0)
    {
      return 4;
    }
    return 1; // Invalid - treat as single byte
  }

  static constexpr bool is_continuation_byte(unsigned char byte) noexcept
  {
    return (byte & 0xC0) == 0x80;
  }

  std::string_view m_str;      ///< Current view
  std::size_t      m_pos;      ///< Current position
  std::string_view m_original; ///< Original string for reset
};

} // namespace fb
