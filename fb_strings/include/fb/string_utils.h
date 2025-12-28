#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace fb
{

// ============================================================================
// Trimming Functions
// ============================================================================

/**
 * @brief Remove leading and trailing whitespace from a string
 * @param str Input string
 * @return Trimmed string
 */
std::string trim(std::string_view str);

/**
 * @brief Remove leading whitespace from a string
 * @param str Input string
 * @return String with leading whitespace removed
 */
std::string trim_left(std::string_view str);

/**
 * @brief Remove trailing whitespace from a string
 * @param str Input string
 * @return String with trailing whitespace removed
 */
std::string trim_right(std::string_view str);

/**
 * @brief Remove leading and trailing occurrences of specified characters
 * @param str Input string
 * @param chars Characters to trim
 * @return Trimmed string
 */
std::string trim(std::string_view str, std::string_view chars);

/**
 * @brief Remove leading occurrences of specified characters
 * @param str Input string
 * @param chars Characters to trim
 * @return String with specified leading characters removed
 */
std::string trim_left(std::string_view str, std::string_view chars);

/**
 * @brief Remove trailing occurrences of specified characters
 * @param str Input string
 * @param chars Characters to trim
 * @return String with specified trailing characters removed
 */
std::string trim_right(std::string_view str, std::string_view chars);

// ============================================================================
// Case Conversion (ASCII-only)
// ============================================================================

/**
 * @brief Convert string to uppercase (ASCII-only: a-z to A-Z)
 * @param str Input string
 * @return Uppercase string
 */
std::string to_upper(std::string_view str);

/**
 * @brief Convert string to lowercase (ASCII-only: A-Z to a-z)
 * @param str Input string
 * @return Lowercase string
 */
std::string to_lower(std::string_view str);

/**
 * @brief Capitalize first character, lowercase the rest (ASCII-only)
 * @param str Input string
 * @return Capitalized string
 */
std::string capitalize(std::string_view str);

/**
 * @brief Capitalize first character of each word (ASCII-only)
 * @param str Input string
 * @return Title-cased string
 */
std::string title_case(std::string_view str);

/**
 * @brief Swap case of all characters (ASCII-only)
 * @param str Input string
 * @return String with swapped case
 */
std::string swap_case(std::string_view str);

// ============================================================================
// Prefix/Suffix Operations
// ============================================================================

/**
 * @brief Check if string starts with the given prefix
 * @param str Input string
 * @param prefix Prefix to check
 * @return true if str starts with prefix
 */
bool starts_with(std::string_view str, std::string_view prefix);

/**
 * @brief Check if string starts with the given character
 * @param str Input string
 * @param ch Character to check
 * @return true if str starts with ch
 */
bool starts_with(std::string_view str, char ch);

/**
 * @brief Check if string ends with the given suffix
 * @param str Input string
 * @param suffix Suffix to check
 * @return true if str ends with suffix
 */
bool ends_with(std::string_view str, std::string_view suffix);

/**
 * @brief Check if string ends with the given character
 * @param str Input string
 * @param ch Character to check
 * @return true if str ends with ch
 */
bool ends_with(std::string_view str, char ch);

/**
 * @brief Remove prefix from string if present
 * @param str Input string
 * @param prefix Prefix to remove
 * @return String without prefix (or original if prefix not found)
 */
std::string remove_prefix(std::string_view str, std::string_view prefix);

/**
 * @brief Remove suffix from string if present
 * @param str Input string
 * @param suffix Suffix to remove
 * @return String without suffix (or original if suffix not found)
 */
std::string remove_suffix(std::string_view str, std::string_view suffix);

/**
 * @brief Add prefix to string if not already present
 * @param str Input string
 * @param prefix Prefix to add
 * @return String with prefix
 */
std::string ensure_prefix(std::string_view str, std::string_view prefix);

/**
 * @brief Add suffix to string if not already present
 * @param str Input string
 * @param suffix Suffix to add
 * @return String with suffix
 */
std::string ensure_suffix(std::string_view str, std::string_view suffix);

// ============================================================================
// Searching
// ============================================================================

/**
 * @brief Check if string contains a substring
 * @param str Input string
 * @param substr Substring to find
 * @return true if substr is found in str
 */
bool contains(std::string_view str, std::string_view substr);

/**
 * @brief Check if string contains a character
 * @param str Input string
 * @param ch Character to find
 * @return true if ch is found in str
 */
bool contains(std::string_view str, char ch);

/**
 * @brief Count occurrences of a substring
 * @param str Input string
 * @param substr Substring to count
 * @return Number of non-overlapping occurrences
 */
size_t count(std::string_view str, std::string_view substr);

/**
 * @brief Count occurrences of a character
 * @param str Input string
 * @param ch Character to count
 * @return Number of occurrences
 */
size_t count(std::string_view str, char ch);

/**
 * @brief Find first occurrence of any character from a set
 * @param str Input string
 * @param chars Characters to search for
 * @return Position of first match, or std::string::npos
 */
size_t find_any(std::string_view str, std::string_view chars);

/**
 * @brief Find last occurrence of any character from a set
 * @param str Input string
 * @param chars Characters to search for
 * @return Position of last match, or std::string::npos
 */
size_t find_any_last(std::string_view str, std::string_view chars);

// ============================================================================
// Replacement
// ============================================================================

/**
 * @brief Replace first occurrence of a substring
 * @param str Input string
 * @param from Substring to find
 * @param to Replacement string
 * @return String with first occurrence replaced
 */
std::string replace(std::string_view str,
                    std::string_view from,
                    std::string_view to);

/**
 * @brief Replace all occurrences of a substring
 * @param str Input string
 * @param from Substring to find
 * @param to Replacement string
 * @return String with all occurrences replaced
 */
std::string replace_all(std::string_view str,
                        std::string_view from,
                        std::string_view to);

/**
 * @brief Replace first occurrence of a character
 * @param str Input string
 * @param from Character to find
 * @param to Replacement character
 * @return String with first occurrence replaced
 */
std::string replace(std::string_view str, char from, char to);

/**
 * @brief Replace all occurrences of a character
 * @param str Input string
 * @param from Character to find
 * @param to Replacement character
 * @return String with all occurrences replaced
 */
std::string replace_all(std::string_view str, char from, char to);

// ============================================================================
// Removal
// ============================================================================

/**
 * @brief Remove first occurrence of a substring
 * @param str Input string
 * @param substr Substring to remove
 * @return String with first occurrence removed
 */
std::string remove(std::string_view str, std::string_view substr);

/**
 * @brief Remove all occurrences of a substring
 * @param str Input string
 * @param substr Substring to remove
 * @return String with all occurrences removed
 */
std::string remove_all(std::string_view str, std::string_view substr);

/**
 * @brief Remove first occurrence of a character
 * @param str Input string
 * @param ch Character to remove
 * @return String with first occurrence removed
 */
std::string remove(std::string_view str, char ch);

/**
 * @brief Remove all occurrences of a character
 * @param str Input string
 * @param ch Character to remove
 * @return String with all occurrences removed
 */
std::string remove_all(std::string_view str, char ch);

// ============================================================================
// Splitting and Joining
// ============================================================================

/**
 * @brief Split string by a character delimiter
 * @param str Input string
 * @param delimiter Character to split on
 * @param keep_empty Whether to keep empty parts (default: true)
 * @return Vector of string parts
 */
std::vector<std::string> split(std::string_view str,
                               char delimiter,
                               bool keep_empty = true);

/**
 * @brief Split string by a string delimiter
 * @param str Input string
 * @param delimiter String to split on
 * @param keep_empty Whether to keep empty parts (default: true)
 * @return Vector of string parts
 */
std::vector<std::string> split(std::string_view str,
                               std::string_view delimiter,
                               bool keep_empty = true);

/**
 * @brief Split string on any character from a set (like strtok)
 * @param str Input string
 * @param delimiters Set of delimiter characters
 * @param keep_empty Whether to keep empty parts (default: false)
 * @return Vector of string parts
 */
std::vector<std::string> split_any(std::string_view str,
                                   std::string_view delimiters,
                                   bool keep_empty = false);

/**
 * @brief Split string into lines (handles CR, LF, and CRLF)
 * @param str Input string
 * @param keep_empty Whether to keep empty lines (default: true)
 * @return Vector of lines
 */
std::vector<std::string> split_lines(std::string_view str,
                                     bool keep_empty = true);

/**
 * @brief Split string on whitespace
 * @param str Input string
 * @return Vector of non-whitespace tokens
 */
std::vector<std::string> split_whitespace(std::string_view str);

/**
 * @brief Split string into at most n parts
 * @param str Input string
 * @param delimiter Character to split on
 * @param max_parts Maximum number of parts (0 = unlimited)
 * @return Vector of string parts
 */
std::vector<std::string> split_n(std::string_view str,
                                 char delimiter,
                                 size_t max_parts);

/**
 * @brief Join strings with a delimiter
 * @param parts Vector of strings to join
 * @param delimiter Delimiter to insert between parts
 * @return Joined string
 */
std::string join(const std::vector<std::string>& parts,
                 std::string_view delimiter);

/**
 * @brief Join strings with a character delimiter
 * @param parts Vector of strings to join
 * @param delimiter Character to insert between parts
 * @return Joined string
 */
std::string join(const std::vector<std::string>& parts, char delimiter);

// ============================================================================
// Substring Extraction
// ============================================================================

/**
 * @brief Extract left n characters
 * @param str Input string
 * @param n Number of characters
 * @return Left n characters (or entire string if shorter)
 */
std::string left(std::string_view str, size_t n);

/**
 * @brief Extract right n characters
 * @param str Input string
 * @param n Number of characters
 * @return Right n characters (or entire string if shorter)
 */
std::string right(std::string_view str, size_t n);

/**
 * @brief Extract substring from middle
 * @param str Input string
 * @param start Start position
 * @param length Number of characters (default: rest of string)
 * @return Substring
 */
std::string mid(std::string_view str,
                size_t start,
                size_t length = std::string::npos);

/**
 * @brief Python-style string slicing with negative index support
 * @param str Input string
 * @param start Start index (negative counts from end)
 * @param end End index (negative counts from end, default: end of string)
 * @return Sliced substring
 *
 * Examples:
 *   slice("hello", 1, 4)   -> "ell"
 *   slice("hello", -3)     -> "llo"
 *   slice("hello", 0, -1)  -> "hell"
 *   slice("hello", -3, -1) -> "ll"
 */
std::string slice(std::string_view str,
                  int start,
                  int end = std::numeric_limits<int>::max());

/**
 * @brief Truncate string to maximum length with optional ellipsis
 * @param str Input string
 * @param max_length Maximum length (including ellipsis if added)
 * @param ellipsis Ellipsis string (default: "...")
 * @return Truncated string
 */
std::string truncate(std::string_view str,
                     size_t max_length,
                     std::string_view ellipsis = "...");

// ============================================================================
// Padding and Alignment
// ============================================================================

/**
 * @brief Pad string on the left to reach target width
 * @param str Input string
 * @param width Target width
 * @param fill Fill character (default: space)
 * @return Padded string
 */
std::string pad_left(std::string_view str, size_t width, char fill = ' ');

/**
 * @brief Pad string on the right to reach target width
 * @param str Input string
 * @param width Target width
 * @param fill Fill character (default: space)
 * @return Padded string
 */
std::string pad_right(std::string_view str, size_t width, char fill = ' ');

/**
 * @brief Center string within target width
 * @param str Input string
 * @param width Target width
 * @param fill Fill character (default: space)
 * @return Centered string (extra padding goes on right if uneven)
 */
std::string center(std::string_view str, size_t width, char fill = ' ');

/**
 * @brief Repeat string n times
 * @param str Input string
 * @param n Number of repetitions
 * @return Repeated string
 */
std::string repeat(std::string_view str, size_t n);

// ============================================================================
// Validation and Type Checking
// ============================================================================

/**
 * @brief Check if string is empty
 * @param str Input string
 * @return true if string is empty
 */
bool is_empty(std::string_view str);

/**
 * @brief Check if string is empty or contains only whitespace
 * @param str Input string
 * @return true if string is empty or all whitespace
 */
bool is_blank(std::string_view str);

/**
 * @brief Check if string contains only ASCII digits (0-9)
 * @param str Input string
 * @return true if all characters are digits
 */
bool is_numeric(std::string_view str);

/**
 * @brief Check if string represents a valid integer
 * @param str Input string
 * @return true if string is a valid integer (optional sign followed by digits)
 */
bool is_integer(std::string_view str);

/**
 * @brief Check if string represents a valid floating-point number
 * @param str Input string
 * @return true if string is a valid float
 */
bool is_float(std::string_view str);

/**
 * @brief Check if string contains only ASCII letters (a-z, A-Z)
 * @param str Input string
 * @return true if all characters are letters
 */
bool is_alpha(std::string_view str);

/**
 * @brief Check if string contains only ASCII letters and digits
 * @param str Input string
 * @return true if all characters are alphanumeric
 */
bool is_alphanumeric(std::string_view str);

/**
 * @brief Check if string is a valid hexadecimal number
 * @param str Input string
 * @param allow_prefix Allow 0x/0X prefix
 * @return true if string is valid hex
 */
bool is_hex(std::string_view str, bool allow_prefix = true);

/**
 * @brief Check if string is a valid binary number
 * @param str Input string
 * @param allow_prefix Allow 0b/0B prefix
 * @return true if string is valid binary
 */
bool is_binary(std::string_view str, bool allow_prefix = true);

/**
 * @brief Check if string is a valid octal number
 * @param str Input string
 * @param allow_prefix Allow 0o/0O or leading 0 prefix
 * @return true if string is valid octal
 */
bool is_octal(std::string_view str, bool allow_prefix = true);

/**
 * @brief Check if string contains only ASCII characters (0-127)
 * @param str Input string
 * @return true if all characters are ASCII
 */
bool is_ascii(std::string_view str);

/**
 * @brief Check if string contains only printable ASCII characters (32-126)
 * @param str Input string
 * @return true if all characters are printable
 */
bool is_printable(std::string_view str);

/**
 * @brief Check if string is all uppercase (ASCII)
 * @param str Input string
 * @return true if all letters are uppercase
 */
bool is_upper(std::string_view str);

/**
 * @brief Check if string is all lowercase (ASCII)
 * @param str Input string
 * @return true if all letters are lowercase
 */
bool is_lower(std::string_view str);

/**
 * @brief Check if string is a valid identifier (letter/underscore followed
 *        by letters, digits, or underscores)
 * @param str Input string
 * @return true if string is a valid identifier
 */
bool is_identifier(std::string_view str);

// ============================================================================
// Number Parsing (returns std::optional for safe error handling)
// ============================================================================

/**
 * @brief Parse string as integer
 * @param str Input string
 * @return Parsed integer or std::nullopt on failure
 */
std::optional<int> to_int(std::string_view str);

/**
 * @brief Parse string as integer with specified base
 * @param str Input string
 * @param base Numeric base (2-36, or 0 for auto-detect)
 * @return Parsed integer or std::nullopt on failure
 */
std::optional<int> to_int(std::string_view str, int base);

/**
 * @brief Parse string as long
 * @param str Input string
 * @return Parsed long or std::nullopt on failure
 */
std::optional<long> to_long(std::string_view str);

/**
 * @brief Parse string as long long
 * @param str Input string
 * @return Parsed long long or std::nullopt on failure
 */
std::optional<long long> to_llong(std::string_view str);

/**
 * @brief Parse string as unsigned long
 * @param str Input string
 * @return Parsed unsigned long or std::nullopt on failure
 */
std::optional<unsigned long> to_ulong(std::string_view str);

/**
 * @brief Parse string as unsigned long long
 * @param str Input string
 * @return Parsed unsigned long long or std::nullopt on failure
 */
std::optional<unsigned long long> to_ullong(std::string_view str);

/**
 * @brief Parse string as float
 * @param str Input string
 * @return Parsed float or std::nullopt on failure
 */
std::optional<float> to_float(std::string_view str);

/**
 * @brief Parse string as double
 * @param str Input string
 * @return Parsed double or std::nullopt on failure
 */
std::optional<double> to_double(std::string_view str);

// ============================================================================
// Number to String Conversion
// ============================================================================

/**
 * @brief Convert integer to string
 * @param value Integer value
 * @return String representation
 */
std::string from_int(int value);

/**
 * @brief Convert integer to string with specified base
 * @param value Integer value
 * @param base Numeric base (2-36)
 * @param uppercase Use uppercase letters for bases > 10
 * @return String representation
 */
std::string from_int(int value, int base, bool uppercase = false);

/**
 * @brief Convert long to string
 * @param value Long value
 * @return String representation
 */
std::string from_long(long value);

/**
 * @brief Convert double to string
 * @param value Double value
 * @param precision Decimal precision (default: 6)
 * @return String representation
 */
std::string from_double(double value, int precision = 6);

/**
 * @brief Convert double to string with fixed notation
 * @param value Double value
 * @param precision Decimal precision
 * @return String representation
 */
std::string from_double_fixed(double value, int precision);

/**
 * @brief Convert double to string with scientific notation
 * @param value Double value
 * @param precision Decimal precision
 * @return String representation
 */
std::string from_double_scientific(double value, int precision);

// ============================================================================
// Encoding and Decoding
// ============================================================================

/**
 * @brief Convert binary data to hexadecimal string
 * @param data Input data
 * @param uppercase Use uppercase hex digits
 * @return Hexadecimal string
 */
std::string to_hex(std::string_view data, bool uppercase = false);

/**
 * @brief Convert hexadecimal string to binary data
 * @param hex Hexadecimal string (with or without 0x prefix)
 * @return Decoded data or std::nullopt on invalid input
 */
std::optional<std::string> from_hex(std::string_view hex);

/**
 * @brief Encode binary data to Base64
 * @param data Input data
 * @return Base64 encoded string
 */
std::string to_base64(std::string_view data);

/**
 * @brief Decode Base64 string to binary data
 * @param base64 Base64 encoded string
 * @return Decoded data or std::nullopt on invalid input
 */
std::optional<std::string> from_base64(std::string_view base64);

/**
 * @brief URL-encode a string (percent encoding)
 * @param str Input string
 * @return URL-encoded string
 */
std::string url_encode(std::string_view str);

/**
 * @brief URL-decode a string (percent decoding)
 * @param str URL-encoded string
 * @return Decoded string or std::nullopt on invalid encoding
 */
std::optional<std::string> url_decode(std::string_view str);

/**
 * @brief Escape special HTML characters
 * @param str Input string
 * @return HTML-escaped string (&, <, >, ", ' escaped)
 */
std::string html_escape(std::string_view str);

/**
 * @brief Unescape HTML entities
 * @param str HTML-escaped string
 * @return Unescaped string
 */
std::string html_unescape(std::string_view str);

/**
 * @brief Escape special JSON characters in a string value
 * @param str Input string
 * @return JSON-escaped string (without surrounding quotes)
 */
std::string json_escape(std::string_view str);

/**
 * @brief Unescape JSON string value
 * @param str JSON-escaped string (without surrounding quotes)
 * @return Unescaped string or std::nullopt on invalid escape sequence
 */
std::optional<std::string> json_unescape(std::string_view str);

// ============================================================================
// Comparison
// ============================================================================

/**
 * @brief Case-insensitive comparison (ASCII-only)
 * @param a First string
 * @param b Second string
 * @return <0 if a<b, 0 if a==b, >0 if a>b
 */
int compare_ignore_case(std::string_view a, std::string_view b);

/**
 * @brief Case-insensitive equality check (ASCII-only)
 * @param a First string
 * @param b Second string
 * @return true if strings are equal ignoring case
 */
bool equals_ignore_case(std::string_view a, std::string_view b);

/**
 * @brief Natural sort comparison (handles embedded numbers)
 * @param a First string
 * @param b Second string
 * @return <0 if a<b, 0 if a==b, >0 if a>b
 *
 * Example: "file2" < "file10" (unlike lexicographic order)
 */
int natural_compare(std::string_view a, std::string_view b);

// ============================================================================
// Miscellaneous Utilities
// ============================================================================

/**
 * @brief Reverse a string
 * @param str Input string
 * @return Reversed string
 */
std::string reverse(std::string_view str);

/**
 * @brief Remove consecutive duplicate characters
 * @param str Input string
 * @return String with consecutive duplicates removed
 */
std::string squeeze(std::string_view str);

/**
 * @brief Remove consecutive duplicate occurrences of a specific character
 * @param str Input string
 * @param ch Character to squeeze
 * @return String with consecutive ch occurrences reduced to one
 */
std::string squeeze(std::string_view str, char ch);

/**
 * @brief Normalize whitespace (collapse consecutive whitespace to single space)
 * @param str Input string
 * @return Normalized string
 */
std::string normalize_whitespace(std::string_view str);

/**
 * @brief Wrap text at specified width
 * @param str Input string
 * @param width Maximum line width
 * @param break_words Break words that exceed width (default: false)
 * @return Wrapped text with newlines
 */
std::string word_wrap(std::string_view str,
                      size_t width,
                      bool break_words = false);

/**
 * @brief Indent each line of text
 * @param str Input string
 * @param indent Indentation string
 * @return Indented text
 */
std::string indent(std::string_view str, std::string_view indent_str);

/**
 * @brief Remove common leading indentation from all lines
 * @param str Input string
 * @return Dedented text
 */
std::string dedent(std::string_view str);

/**
 * @brief Get common prefix of two strings
 * @param a First string
 * @param b Second string
 * @return Common prefix
 */
std::string common_prefix(std::string_view a, std::string_view b);

/**
 * @brief Get common suffix of two strings
 * @param a First string
 * @param b Second string
 * @return Common suffix
 */
std::string common_suffix(std::string_view a, std::string_view b);

/**
 * @brief Calculate Levenshtein distance between two strings
 * @param a First string
 * @param b Second string
 * @return Edit distance
 */
size_t levenshtein_distance(std::string_view a, std::string_view b);

} // namespace fb
