/// @file string_random.cpp
/// @brief Implementation of random string generation utilities

#include "fb/string_random.h"

#include <random>
#include <stdexcept>

namespace fb
{

namespace
{

// ============================================================================
// Thread-Local Random Engine
// ============================================================================

/// @brief Get thread-local random engine
std::mt19937_64& get_random_engine()
{
  thread_local std::mt19937_64 engine{std::random_device{}()};
  return engine;
}

// ============================================================================
// Character Sets
// ============================================================================

constexpr std::string_view CHARSET_ALPHANUMERIC =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

constexpr std::string_view CHARSET_ALPHABETIC = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

constexpr std::string_view CHARSET_NUMERIC = "0123456789";

constexpr std::string_view CHARSET_HEX_LOWER = "0123456789abcdef";

constexpr std::string_view CHARSET_HEX_UPPER = "0123456789ABCDEF";

} // namespace

// ============================================================================
// Random String Functions
// ============================================================================

std::string random_string(std::size_t length, std::string_view charset)
{
  if (charset.empty())
  {
    throw std::invalid_argument("charset must not be empty");
  }

  if (length == 0)
  {
    return {};
  }

  std::string                             result;
  result.reserve(length);

  auto& engine = get_random_engine();
  std::uniform_int_distribution<std::size_t> dist(0, charset.size() - 1);

  for (std::size_t i = 0; i < length; ++i)
  {
    result.push_back(charset[dist(engine)]);
  }

  return result;
}

std::string random_alphanumeric(std::size_t length)
{
  return random_string(length, CHARSET_ALPHANUMERIC);
}

std::string random_alphabetic(std::size_t length)
{
  return random_string(length, CHARSET_ALPHABETIC);
}

std::string random_numeric(std::size_t length)
{
  return random_string(length, CHARSET_NUMERIC);
}

std::string random_hex(std::size_t length, bool uppercase)
{
  return random_string(length, uppercase ? CHARSET_HEX_UPPER : CHARSET_HEX_LOWER);
}

std::string random_uuid()
{
  // UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
  // Total: 36 characters (32 hex + 4 hyphens)
  // Version 4 = bits 12-15 of time_hi_and_version set to 0100
  // Variant = bits 6-7 of clock_seq_hi_and_reserved set to 10

  auto& engine = get_random_engine();
  std::uniform_int_distribution<std::uint32_t> dist(0, 15);

  std::string uuid;
  uuid.reserve(36);

  constexpr char hex_chars[] = "0123456789abcdef";

  // Generate 32 random hex digits with proper version and variant bits
  for (int i = 0; i < 36; ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      // Insert hyphens at positions 8, 13, 18, 23
      uuid.push_back('-');
    }
    else if (i == 14)
    {
      // Version 4: position 14 (index in hex chars is 14, after 8-xxxx- which is position 12-13)
      // The 13th hex digit (0-indexed) should be '4'
      uuid.push_back('4');
    }
    else if (i == 19)
    {
      // Variant: position 19, must be 8, 9, a, or b (binary 10xx)
      // The 17th hex digit (0-indexed) should be 8-b
      uuid.push_back(hex_chars[8 + (dist(engine) & 0x3)]);
    }
    else
    {
      uuid.push_back(hex_chars[dist(engine)]);
    }
  }

  return uuid;
}

} // namespace fb
