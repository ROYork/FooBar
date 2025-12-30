/// @file string_hash.h
/// @brief Non-cryptographic string hash functions
///
/// Provides fast, non-cryptographic hash functions for string hashing.
/// These are suitable for hash tables, caching, and data integrity checks,
/// but NOT for security-critical applications.
///
/// Available hash functions:
/// - FNV-1a (32-bit and 64-bit): Fast, good distribution
/// - DJB2: Simple, widely used
/// - Case-insensitive hash: For case-insensitive lookups (ASCII only)
/// - CRC-32: IEEE polynomial, for checksums
///
/// Thread Safety:
/// - All functions are thread-safe (pure functions with no shared state)
///
/// Example:
/// @code
/// std::string key = "hello";
/// uint32_t hash = fb::hash_fnv1a_32(key);
///
/// // Case-insensitive lookup
/// uint32_t h1 = fb::hash_case_insensitive("Hello");
/// uint32_t h2 = fb::hash_case_insensitive("HELLO");
/// // h1 == h2
/// @endcode

#pragma once

#include <cstdint>
#include <string_view>

namespace fb
{

/// @brief FNV-1a 32-bit hash
///
/// Fast hash function with good distribution properties.
/// Uses the Fowler-Noll-Vo hash algorithm variant 1a.
///
/// @param str String to hash
/// @return 32-bit hash value
[[nodiscard]] std::uint32_t hash_fnv1a_32(std::string_view str) noexcept;

/// @brief FNV-1a 64-bit hash
///
/// 64-bit variant of FNV-1a for reduced collision probability.
///
/// @param str String to hash
/// @return 64-bit hash value
[[nodiscard]] std::uint64_t hash_fnv1a_64(std::string_view str) noexcept;

/// @brief DJB2 hash
///
/// Daniel J. Bernstein's hash function. Simple and fast.
///
/// @param str String to hash
/// @return 32-bit hash value
[[nodiscard]] std::uint32_t hash_djb2(std::string_view str) noexcept;

/// @brief Case-insensitive hash (ASCII only)
///
/// Produces the same hash for strings that differ only in ASCII case.
/// Uses FNV-1a internally with lowercase conversion.
///
/// @param str String to hash
/// @return 32-bit hash value
[[nodiscard]] std::uint32_t hash_case_insensitive(std::string_view str) noexcept;

/// @brief CRC-32 checksum (IEEE polynomial)
///
/// Calculates CRC-32 using the IEEE 802.3 polynomial.
/// Suitable for data integrity checks.
///
/// @param str String to compute checksum for
/// @return 32-bit CRC value
[[nodiscard]] std::uint32_t crc32(std::string_view str) noexcept;

/// @brief CRC-32 checksum for binary data
///
/// @param data Pointer to data
/// @param length Length of data in bytes
/// @return 32-bit CRC value
[[nodiscard]] std::uint32_t crc32(const void* data, std::size_t length) noexcept;

} // namespace fb
