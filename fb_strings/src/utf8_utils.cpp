/// @file utf8_utils.cpp
/// @brief Implementation of UTF-8 string manipulation utilities

#include "fb/utf8_utils.h"

namespace fb
{

namespace
{

/// @brief Get the byte length of a UTF-8 code point from its first byte
///
/// @param first_byte First byte of the code point
/// @return Number of bytes in the code point (1-4), or 0 if invalid
constexpr std::size_t codepoint_byte_length(unsigned char first_byte) noexcept
{
  if ((first_byte & 0x80) == 0x00)
  {
    return 1; // 0xxxxxxx - ASCII
  }
  if ((first_byte & 0xE0) == 0xC0)
  {
    return 2; // 110xxxxx
  }
  if ((first_byte & 0xF0) == 0xE0)
  {
    return 3; // 1110xxxx
  }
  if ((first_byte & 0xF8) == 0xF0)
  {
    return 4; // 11110xxx
  }
  return 0; // Invalid leading byte
}

/// @brief Check if byte is a valid UTF-8 continuation byte
constexpr bool is_continuation_byte(unsigned char byte) noexcept
{
  return (byte & 0xC0) == 0x80; // 10xxxxxx
}

/// @brief Check if a code point sequence is valid
///
/// Validates that continuation bytes are correct and the code point
/// value is valid (not overlong encoding, not surrogate, in range).
bool is_valid_codepoint_sequence(const unsigned char* bytes, std::size_t length) noexcept
{
  if (length == 0)
  {
    return false;
  }

  // Check continuation bytes
  for (std::size_t i = 1; i < length; ++i)
  {
    if (!is_continuation_byte(bytes[i]))
    {
      return false;
    }
  }

  // Decode and validate the code point value
  std::uint32_t codepoint = 0;

  switch (length)
  {
    case 1:
      codepoint = bytes[0];
      break;
    case 2:
      codepoint = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
      // Check for overlong encoding
      if (codepoint < 0x80)
      {
        return false;
      }
      break;
    case 3:
      codepoint = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
      // Check for overlong encoding
      if (codepoint < 0x800)
      {
        return false;
      }
      // Check for surrogate pairs (U+D800 to U+DFFF)
      if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
      {
        return false;
      }
      break;
    case 4:
      codepoint = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) |
                  (bytes[3] & 0x3F);
      // Check for overlong encoding
      if (codepoint < 0x10000)
      {
        return false;
      }
      // Check for values beyond Unicode range
      if (codepoint > 0x10FFFF)
      {
        return false;
      }
      break;
    default:
      return false;
  }

  return true;
}

} // namespace

// ============================================================================
// UTF-8 Length Functions
// ============================================================================

std::size_t utf8_length(std::string_view str) noexcept
{
  std::size_t count = 0;
  std::size_t i     = 0;

  while (i < str.size())
  {
    auto first_byte = static_cast<unsigned char>(str[i]);
    auto cp_len     = codepoint_byte_length(first_byte);

    if (cp_len == 0 || i + cp_len > str.size())
    {
      // Invalid or truncated sequence - count as 1
      ++i;
    }
    else
    {
      i += cp_len;
    }
    ++count;
  }

  return count;
}

// ============================================================================
// UTF-8 Indexing Functions
// ============================================================================

std::string utf8_at(std::string_view str, std::size_t index)
{
  std::size_t byte_pos = utf8_byte_offset(str, index);
  if (byte_pos == std::string::npos || byte_pos >= str.size())
  {
    return {};
  }

  auto first_byte = static_cast<unsigned char>(str[byte_pos]);
  auto cp_len     = codepoint_byte_length(first_byte);

  if (cp_len == 0 || byte_pos + cp_len > str.size())
  {
    // Invalid or truncated - return single byte
    return std::string(1, str[byte_pos]);
  }

  return std::string(str.substr(byte_pos, cp_len));
}

std::size_t utf8_byte_offset(std::string_view str, std::size_t codepoint_index) noexcept
{
  std::size_t cp_count = 0;
  std::size_t i        = 0;

  while (i < str.size() && cp_count < codepoint_index)
  {
    auto first_byte = static_cast<unsigned char>(str[i]);
    auto cp_len     = codepoint_byte_length(first_byte);

    if (cp_len == 0 || i + cp_len > str.size())
    {
      ++i; // Skip invalid byte
    }
    else
    {
      i += cp_len;
    }
    ++cp_count;
  }

  if (cp_count == codepoint_index && i <= str.size())
  {
    return i;
  }

  return std::string::npos;
}

std::size_t utf8_codepoint_index(std::string_view str, std::size_t byte_offset) noexcept
{
  if (byte_offset > str.size())
  {
    return std::string::npos;
  }

  std::size_t cp_count = 0;
  std::size_t i        = 0;

  while (i < byte_offset && i < str.size())
  {
    auto first_byte = static_cast<unsigned char>(str[i]);
    auto cp_len     = codepoint_byte_length(first_byte);

    if (cp_len == 0 || i + cp_len > str.size())
    {
      ++i;
    }
    else
    {
      i += cp_len;
    }
    ++cp_count;
  }

  return cp_count;
}

// ============================================================================
// UTF-8 Substring Functions
// ============================================================================

std::string utf8_left(std::string_view str, std::size_t n)
{
  std::size_t byte_pos = utf8_byte_offset(str, n);
  if (byte_pos == std::string::npos)
  {
    return std::string(str);
  }
  return std::string(str.substr(0, byte_pos));
}

std::string utf8_right(std::string_view str, std::size_t n)
{
  std::size_t total_cps = utf8_length(str);
  if (n >= total_cps)
  {
    return std::string(str);
  }

  std::size_t start_cp = total_cps - n;
  return utf8_mid(str, start_cp);
}

std::string utf8_mid(std::string_view str, std::size_t pos, std::size_t count)
{
  std::size_t start_byte = utf8_byte_offset(str, pos);
  if (start_byte == std::string::npos)
  {
    return {};
  }

  if (count == std::string::npos)
  {
    return std::string(str.substr(start_byte));
  }

  // Find end position
  std::size_t end_byte = utf8_byte_offset(str.substr(start_byte), count);
  if (end_byte == std::string::npos)
  {
    return std::string(str.substr(start_byte));
  }

  return std::string(str.substr(start_byte, end_byte));
}

// ============================================================================
// UTF-8 Validation Functions
// ============================================================================

bool is_valid_utf8(std::string_view str) noexcept
{
  std::size_t i = 0;

  while (i < str.size())
  {
    auto first_byte = static_cast<unsigned char>(str[i]);
    auto cp_len     = codepoint_byte_length(first_byte);

    if (cp_len == 0)
    {
      // Invalid leading byte
      return false;
    }

    if (i + cp_len > str.size())
    {
      // Truncated sequence
      return false;
    }

    // Validate the complete sequence
    if (!is_valid_codepoint_sequence(reinterpret_cast<const unsigned char*>(str.data() + i), cp_len))
    {
      return false;
    }

    i += cp_len;
  }

  return true;
}

std::string sanitize_utf8(std::string_view str, char replacement)
{
  std::string result;
  result.reserve(str.size());

  std::size_t i = 0;

  while (i < str.size())
  {
    auto first_byte = static_cast<unsigned char>(str[i]);
    auto cp_len     = codepoint_byte_length(first_byte);

    if (cp_len == 0)
    {
      // Invalid leading byte
      result.push_back(replacement);
      ++i;
      continue;
    }

    if (i + cp_len > str.size())
    {
      // Truncated sequence - replace remaining bytes
      while (i < str.size())
      {
        result.push_back(replacement);
        ++i;
      }
      break;
    }

    // Validate the complete sequence
    if (is_valid_codepoint_sequence(reinterpret_cast<const unsigned char*>(str.data() + i), cp_len))
    {
      // Valid sequence - copy it
      result.append(str.data() + i, cp_len);
    }
    else
    {
      // Invalid sequence - replace all bytes
      for (std::size_t j = 0; j < cp_len; ++j)
      {
        result.push_back(replacement);
      }
    }

    i += cp_len;
  }

  return result;
}

} // namespace fb
