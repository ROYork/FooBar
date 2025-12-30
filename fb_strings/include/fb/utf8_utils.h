/// @file utf8_utils.h
/// @brief UTF-8 string manipulation utilities
///
/// Provides functions for working with UTF-8 encoded strings at the
/// code point level rather than byte level.
///
/// Features:
/// - Code point counting and indexing
/// - UTF-8 aware substring operations
/// - Validation and sanitization
///
/// Thread Safety:
/// - All functions are thread-safe (pure functions)
///
/// Example:
/// @code
/// std::string text = "Hello, \xE4\xB8\x96\xE7\x95\x8C";  // "Hello, 世界"
///
/// // Get length in code points (not bytes)
/// size_t len = fb::utf8_length(text);  // 9 (not 13)
///
/// // Get substring by code point index
/// std::string left = fb::utf8_left(text, 5);  // "Hello"
/// std::string right = fb::utf8_right(text, 2);  // "世界"
///
/// // Validate UTF-8
/// if (!fb::is_valid_utf8(text)) {
///     text = fb::sanitize_utf8(text);
/// }
/// @endcode

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace fb
{

/// @brief Get length in code points (not bytes)
///
/// Counts the number of Unicode code points in the string.
/// For ASCII strings, this equals the byte length.
///
/// @param str UTF-8 encoded string
/// @return Number of code points
/// @note Invalid UTF-8 sequences are counted as 1 code point per byte
[[nodiscard]] std::size_t utf8_length(std::string_view str) noexcept;

/// @brief Get code point at index
///
/// Returns the UTF-8 encoded code point at the specified index.
///
/// @param str UTF-8 encoded string
/// @param index Zero-based code point index
/// @return UTF-8 encoded code point as string, or empty if index out of range
[[nodiscard]] std::string utf8_at(std::string_view str, std::size_t index);

/// @brief Get leftmost n code points
///
/// @param str UTF-8 encoded string
/// @param n Number of code points to return
/// @return First n code points (or entire string if n >= length)
[[nodiscard]] std::string utf8_left(std::string_view str, std::size_t n);

/// @brief Get rightmost n code points
///
/// @param str UTF-8 encoded string
/// @param n Number of code points to return
/// @return Last n code points (or entire string if n >= length)
[[nodiscard]] std::string utf8_right(std::string_view str, std::size_t n);

/// @brief Get substring by code point indices
///
/// @param str UTF-8 encoded string
/// @param pos Starting code point index
/// @param count Maximum number of code points (default: all remaining)
/// @return Substring from pos for count code points
[[nodiscard]] std::string utf8_mid(std::string_view str, std::size_t pos,
                                    std::size_t count = std::string::npos);

/// @brief Check if string is valid UTF-8
///
/// Validates that the string contains only valid UTF-8 sequences.
///
/// @param str String to validate
/// @return true if string is valid UTF-8
[[nodiscard]] bool is_valid_utf8(std::string_view str) noexcept;

/// @brief Sanitize invalid UTF-8 sequences
///
/// Replaces invalid UTF-8 bytes with a replacement character.
///
/// @param str String to sanitize
/// @param replacement Character to use for invalid bytes (default: '?')
/// @return Sanitized string with valid UTF-8
[[nodiscard]] std::string sanitize_utf8(std::string_view str, char replacement = '?');

/// @brief Get byte position of code point index
///
/// Converts a code point index to a byte offset.
///
/// @param str UTF-8 encoded string
/// @param codepoint_index Code point index
/// @return Byte offset, or string::npos if index out of range
[[nodiscard]] std::size_t utf8_byte_offset(std::string_view str, std::size_t codepoint_index) noexcept;

/// @brief Get code point index from byte position
///
/// Converts a byte offset to a code point index.
///
/// @param str UTF-8 encoded string
/// @param byte_offset Byte offset
/// @return Code point index, or string::npos if offset invalid
[[nodiscard]] std::size_t utf8_codepoint_index(std::string_view str, std::size_t byte_offset) noexcept;

} // namespace fb
