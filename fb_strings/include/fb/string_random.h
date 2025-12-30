/// @file string_random.h
/// @brief Random string generation utilities
///
/// Provides functions for generating random strings for testing,
/// token generation, and unique identifiers.
///
/// Features:
/// - Custom character set support
/// - Convenience functions for common patterns
/// - RFC-4122 UUID v4 generation
/// - Thread-safe (uses thread-local random engine)
///
/// Thread Safety:
/// - All functions are thread-safe
/// - Each thread maintains its own random engine
///
/// Example:
/// @code
/// // Generate random alphanumeric string
/// std::string token = fb::random_alphanumeric(32);
///
/// // Generate UUID
/// std::string uuid = fb::random_uuid();  // e.g., "550e8400-e29b-41d4-a716-446655440000"
///
/// // Custom character set
/// std::string code = fb::random_string(6, "ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
/// @endcode

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace fb
{

/// @brief Generate random string from custom character set
///
/// Creates a random string of specified length using characters
/// from the provided character set.
///
/// @param length Number of characters to generate
/// @param charset Characters to choose from (must not be empty)
/// @return Random string of specified length
/// @throw std::invalid_argument if charset is empty
std::string random_string(std::size_t length, std::string_view charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

/// @brief Generate random alphanumeric string
///
/// Creates a random string using A-Z, a-z, and 0-9.
///
/// @param length Number of characters to generate
/// @return Random alphanumeric string
std::string random_alphanumeric(std::size_t length);

/// @brief Generate random alphabetic string
///
/// Creates a random string using A-Z and a-z only.
///
/// @param length Number of characters to generate
/// @return Random alphabetic string
std::string random_alphabetic(std::size_t length);

/// @brief Generate random numeric string
///
/// Creates a random string using 0-9 only.
///
/// @param length Number of characters to generate
/// @return Random numeric string
std::string random_numeric(std::size_t length);

/// @brief Generate random hexadecimal string
///
/// Creates a random string using 0-9 and a-f (or A-F if uppercase).
///
/// @param length Number of characters to generate
/// @param uppercase Use uppercase hex digits (A-F) if true
/// @return Random hexadecimal string
std::string random_hex(std::size_t length, bool uppercase = false);

/// @brief Generate RFC-4122 UUID version 4 (random)
///
/// Creates a random UUID in the standard format:
/// xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
/// where x is a random hex digit and y is one of 8, 9, a, or b.
///
/// @return Random UUID string (36 characters with hyphens)
std::string random_uuid();

} // namespace fb
