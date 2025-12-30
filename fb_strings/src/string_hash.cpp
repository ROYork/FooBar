/// @file string_hash.cpp
/// @brief Implementation of non-cryptographic string hash functions

#include "fb/string_hash.h"

#include <array>

namespace fb
{

namespace
{

// ============================================================================
// FNV-1a Constants
// ============================================================================

// FNV-1a 32-bit constants
constexpr std::uint32_t FNV1A_32_PRIME  = 0x01000193;
constexpr std::uint32_t FNV1A_32_OFFSET = 0x811c9dc5;

// FNV-1a 64-bit constants
constexpr std::uint64_t FNV1A_64_PRIME  = 0x00000100000001B3ULL;
constexpr std::uint64_t FNV1A_64_OFFSET = 0xcbf29ce484222325ULL;

// ============================================================================
// CRC-32 Lookup Table (IEEE 802.3 polynomial: 0xEDB88320)
// ============================================================================

// Pre-computed CRC-32 lookup table
constexpr std::array<std::uint32_t, 256> make_crc_table()
{
  std::array<std::uint32_t, 256> table{};
  constexpr std::uint32_t        polynomial = 0xEDB88320;

  for (std::uint32_t i = 0; i < 256; ++i)
  {
    std::uint32_t crc = i;
    for (int j = 0; j < 8; ++j)
    {
      if (crc & 1)
      {
        crc = (crc >> 1) ^ polynomial;
      }
      else
      {
        crc >>= 1;
      }
    }
    table[i] = crc;
  }
  return table;
}

constexpr auto CRC_TABLE = make_crc_table();

// ============================================================================
// Helper Functions
// ============================================================================

/// @brief Convert ASCII character to lowercase
constexpr char to_lower_ascii(char c) noexcept
{
  if (c >= 'A' && c <= 'Z')
  {
    return static_cast<char>(c + ('a' - 'A'));
  }
  return c;
}

} // namespace

// ============================================================================
// FNV-1a Hash Functions
// ============================================================================

std::uint32_t hash_fnv1a_32(std::string_view str) noexcept
{
  std::uint32_t hash = FNV1A_32_OFFSET;

  for (char c : str)
  {
    hash ^= static_cast<std::uint8_t>(c);
    hash *= FNV1A_32_PRIME;
  }

  return hash;
}

std::uint64_t hash_fnv1a_64(std::string_view str) noexcept
{
  std::uint64_t hash = FNV1A_64_OFFSET;

  for (char c : str)
  {
    hash ^= static_cast<std::uint8_t>(c);
    hash *= FNV1A_64_PRIME;
  }

  return hash;
}

// ============================================================================
// DJB2 Hash Function
// ============================================================================

std::uint32_t hash_djb2(std::string_view str) noexcept
{
  std::uint32_t hash = 5381;

  for (char c : str)
  {
    // hash * 33 + c
    hash = ((hash << 5) + hash) + static_cast<std::uint8_t>(c);
  }

  return hash;
}

// ============================================================================
// Case-Insensitive Hash
// ============================================================================

std::uint32_t hash_case_insensitive(std::string_view str) noexcept
{
  std::uint32_t hash = FNV1A_32_OFFSET;

  for (char c : str)
  {
    hash ^= static_cast<std::uint8_t>(to_lower_ascii(c));
    hash *= FNV1A_32_PRIME;
  }

  return hash;
}

// ============================================================================
// CRC-32
// ============================================================================

std::uint32_t crc32(std::string_view str) noexcept
{
  return crc32(str.data(), str.size());
}

std::uint32_t crc32(const void* data, std::size_t length) noexcept
{
  const auto*   bytes = static_cast<const std::uint8_t*>(data);
  std::uint32_t crc   = 0xFFFFFFFF;

  for (std::size_t i = 0; i < length; ++i)
  {
    std::uint8_t index = static_cast<std::uint8_t>(crc ^ bytes[i]);
    crc                = (crc >> 8) ^ CRC_TABLE[index];
  }

  return crc ^ 0xFFFFFFFF;
}

} // namespace fb
