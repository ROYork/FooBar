#include <fb/string_utils.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>

namespace fb
{

// ============================================================================
// Internal Helpers
// ============================================================================

namespace
{

constexpr bool is_whitespace(char ch) noexcept
{
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' ||
         ch == '\v';
}

constexpr bool is_ascii_upper(char ch) noexcept
{
  return ch >= 'A' && ch <= 'Z';
}

constexpr bool is_ascii_lower(char ch) noexcept
{
  return ch >= 'a' && ch <= 'z';
}

constexpr bool is_ascii_digit(char ch) noexcept
{
  return ch >= '0' && ch <= '9';
}

constexpr bool is_ascii_alpha(char ch) noexcept
{
  return is_ascii_upper(ch) || is_ascii_lower(ch);
}

constexpr char to_ascii_upper(char ch) noexcept
{
  return is_ascii_lower(ch) ? static_cast<char>(ch - 'a' + 'A') : ch;
}

constexpr char to_ascii_lower(char ch) noexcept
{
  return is_ascii_upper(ch) ? static_cast<char>(ch - 'A' + 'a') : ch;
}

constexpr bool is_hex_digit(char ch) noexcept
{
  return is_ascii_digit(ch) || (ch >= 'a' && ch <= 'f') ||
         (ch >= 'A' && ch <= 'F');
}

constexpr int hex_value(char ch) noexcept
{
  if (is_ascii_digit(ch))
  {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f')
  {
    return ch - 'a' + 10;
  }
  if (ch >= 'A' && ch <= 'F')
  {
    return ch - 'A' + 10;
  }
  return -1;
}

constexpr char hex_char(int value, bool uppercase) noexcept
{
  if (value < 10)
  {
    return static_cast<char>('0' + value);
  }
  return static_cast<char>((uppercase ? 'A' : 'a') + value - 10);
}

// Base64 encoding table
constexpr std::string_view BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr int base64_index(char ch) noexcept
{
  if (ch >= 'A' && ch <= 'Z')
  {
    return ch - 'A';
  }
  if (ch >= 'a' && ch <= 'z')
  {
    return ch - 'a' + 26;
  }
  if (ch >= '0' && ch <= '9')
  {
    return ch - '0' + 52;
  }
  if (ch == '+')
  {
    return 62;
  }
  if (ch == '/')
  {
    return 63;
  }
  return -1;
}

} // namespace

// ============================================================================
// Trimming Functions
// ============================================================================

std::string trim(std::string_view str)
{
  size_t start = 0;
  size_t end   = str.size();

  while (start < end && is_whitespace(str[start]))
  {
    ++start;
  }
  while (end > start && is_whitespace(str[end - 1]))
  {
    --end;
  }

  return std::string(str.substr(start, end - start));
}

std::string trim_left(std::string_view str)
{
  size_t start = 0;
  while (start < str.size() && is_whitespace(str[start]))
  {
    ++start;
  }
  return std::string(str.substr(start));
}

std::string trim_right(std::string_view str)
{
  size_t end = str.size();
  while (end > 0 && is_whitespace(str[end - 1]))
  {
    --end;
  }
  return std::string(str.substr(0, end));
}

std::string trim(std::string_view str, std::string_view chars)
{
  size_t start = str.find_first_not_of(chars);
  if (start == std::string_view::npos)
  {
    return std::string();
  }
  size_t end = str.find_last_not_of(chars);
  return std::string(str.substr(start, end - start + 1));
}

std::string trim_left(std::string_view str, std::string_view chars)
{
  size_t start = str.find_first_not_of(chars);
  if (start == std::string_view::npos)
  {
    return std::string();
  }
  return std::string(str.substr(start));
}

std::string trim_right(std::string_view str, std::string_view chars)
{
  size_t end = str.find_last_not_of(chars);
  if (end == std::string_view::npos)
  {
    return std::string();
  }
  return std::string(str.substr(0, end + 1));
}

// ============================================================================
// Case Conversion
// ============================================================================

std::string to_upper(std::string_view str)
{
  std::string result;
  result.reserve(str.size());
  for (char ch : str)
  {
    result.push_back(to_ascii_upper(ch));
  }
  return result;
}

std::string to_lower(std::string_view str)
{
  std::string result;
  result.reserve(str.size());
  for (char ch : str)
  {
    result.push_back(to_ascii_lower(ch));
  }
  return result;
}

std::string capitalize(std::string_view str)
{
  if (str.empty())
  {
    return std::string();
  }
  std::string result;
  result.reserve(str.size());
  result.push_back(to_ascii_upper(str[0]));
  for (size_t i = 1; i < str.size(); ++i)
  {
    result.push_back(to_ascii_lower(str[i]));
  }
  return result;
}

std::string title_case(std::string_view str)
{
  std::string result;
  result.reserve(str.size());
  bool capitalize_next = true;
  for (char ch : str)
  {
    if (is_whitespace(ch) || ch == '-' || ch == '_')
    {
      result.push_back(ch);
      capitalize_next = true;
    }
    else if (capitalize_next)
    {
      result.push_back(to_ascii_upper(ch));
      capitalize_next = false;
    }
    else
    {
      result.push_back(to_ascii_lower(ch));
    }
  }
  return result;
}

std::string swap_case(std::string_view str)
{
  std::string result;
  result.reserve(str.size());
  for (char ch : str)
  {
    if (is_ascii_upper(ch))
    {
      result.push_back(to_ascii_lower(ch));
    }
    else if (is_ascii_lower(ch))
    {
      result.push_back(to_ascii_upper(ch));
    }
    else
    {
      result.push_back(ch);
    }
  }
  return result;
}

// ============================================================================
// Prefix/Suffix Operations
// ============================================================================

bool starts_with(std::string_view str, std::string_view prefix)
{
  if (prefix.size() > str.size())
  {
    return false;
  }
  return str.substr(0, prefix.size()) == prefix;
}

bool starts_with(std::string_view str, char ch)
{
  return !str.empty() && str[0] == ch;
}

bool ends_with(std::string_view str, std::string_view suffix)
{
  if (suffix.size() > str.size())
  {
    return false;
  }
  return str.substr(str.size() - suffix.size()) == suffix;
}

bool ends_with(std::string_view str, char ch)
{
  return !str.empty() && str.back() == ch;
}

std::string remove_prefix(std::string_view str, std::string_view prefix)
{
  if (starts_with(str, prefix))
  {
    return std::string(str.substr(prefix.size()));
  }
  return std::string(str);
}

std::string remove_suffix(std::string_view str, std::string_view suffix)
{
  if (ends_with(str, suffix))
  {
    return std::string(str.substr(0, str.size() - suffix.size()));
  }
  return std::string(str);
}

std::string ensure_prefix(std::string_view str, std::string_view prefix)
{
  if (starts_with(str, prefix))
  {
    return std::string(str);
  }
  std::string result;
  result.reserve(prefix.size() + str.size());
  result.append(prefix);
  result.append(str);
  return result;
}

std::string ensure_suffix(std::string_view str, std::string_view suffix)
{
  if (ends_with(str, suffix))
  {
    return std::string(str);
  }
  std::string result;
  result.reserve(str.size() + suffix.size());
  result.append(str);
  result.append(suffix);
  return result;
}

// ============================================================================
// Searching
// ============================================================================

bool contains(std::string_view str, std::string_view substr)
{
  return str.find(substr) != std::string_view::npos;
}

bool contains(std::string_view str, char ch)
{
  return str.find(ch) != std::string_view::npos;
}

size_t count(std::string_view str, std::string_view substr)
{
  if (substr.empty())
  {
    return 0;
  }
  size_t count = 0;
  size_t pos   = 0;
  while ((pos = str.find(substr, pos)) != std::string_view::npos)
  {
    ++count;
    pos += substr.size();
  }
  return count;
}

size_t count(std::string_view str, char ch)
{
  return static_cast<size_t>(std::count(str.begin(), str.end(), ch));
}

size_t find_any(std::string_view str, std::string_view chars)
{
  return str.find_first_of(chars);
}

size_t find_any_last(std::string_view str, std::string_view chars)
{
  return str.find_last_of(chars);
}

// ============================================================================
// Replacement
// ============================================================================

std::string replace(std::string_view str,
                    std::string_view from,
                    std::string_view to)
{
  if (from.empty())
  {
    return std::string(str);
  }
  size_t pos = str.find(from);
  if (pos == std::string_view::npos)
  {
    return std::string(str);
  }
  std::string result;
  result.reserve(str.size() + to.size() - from.size());
  result.append(str.substr(0, pos));
  result.append(to);
  result.append(str.substr(pos + from.size()));
  return result;
}

std::string replace_all(std::string_view str,
                        std::string_view from,
                        std::string_view to)
{
  if (from.empty())
  {
    return std::string(str);
  }
  std::string result;
  result.reserve(str.size());
  size_t pos      = 0;
  size_t last_pos = 0;
  while ((pos = str.find(from, last_pos)) != std::string_view::npos)
  {
    result.append(str.substr(last_pos, pos - last_pos));
    result.append(to);
    last_pos = pos + from.size();
  }
  result.append(str.substr(last_pos));
  return result;
}

std::string replace(std::string_view str, char from, char to)
{
  std::string result(str);
  for (char& ch : result)
  {
    if (ch == from)
    {
      ch = to;
      break;
    }
  }
  return result;
}

std::string replace_all(std::string_view str, char from, char to)
{
  std::string result(str);
  for (char& ch : result)
  {
    if (ch == from)
    {
      ch = to;
    }
  }
  return result;
}

// ============================================================================
// Removal
// ============================================================================

std::string remove(std::string_view str, std::string_view substr)
{
  return replace(str, substr, "");
}

std::string remove_all(std::string_view str, std::string_view substr)
{
  return replace_all(str, substr, "");
}

std::string remove(std::string_view str, char ch)
{
  std::string result;
  result.reserve(str.size());
  bool found = false;
  for (char c : str)
  {
    if (c == ch && !found)
    {
      found = true;
    }
    else
    {
      result.push_back(c);
    }
  }
  return result;
}

std::string remove_all(std::string_view str, char ch)
{
  std::string result;
  result.reserve(str.size());
  for (char c : str)
  {
    if (c != ch)
    {
      result.push_back(c);
    }
  }
  return result;
}

// ============================================================================
// Splitting and Joining
// ============================================================================

std::vector<std::string> split(std::string_view str,
                               char delimiter,
                               bool keep_empty)
{
  std::vector<std::string> result;
  size_t start = 0;
  size_t pos   = 0;

  while ((pos = str.find(delimiter, start)) != std::string_view::npos)
  {
    if (keep_empty || pos > start)
    {
      result.emplace_back(str.substr(start, pos - start));
    }
    start = pos + 1;
  }

  if (keep_empty || start < str.size())
  {
    result.emplace_back(str.substr(start));
  }

  return result;
}

std::vector<std::string> split(std::string_view str,
                               std::string_view delimiter,
                               bool keep_empty)
{
  std::vector<std::string> result;

  if (delimiter.empty())
  {
    result.emplace_back(str);
    return result;
  }

  size_t start = 0;
  size_t pos   = 0;

  while ((pos = str.find(delimiter, start)) != std::string_view::npos)
  {
    if (keep_empty || pos > start)
    {
      result.emplace_back(str.substr(start, pos - start));
    }
    start = pos + delimiter.size();
  }

  if (keep_empty || start < str.size())
  {
    result.emplace_back(str.substr(start));
  }

  return result;
}

std::vector<std::string> split_any(std::string_view str,
                                   std::string_view delimiters,
                                   bool keep_empty)
{
  std::vector<std::string> result;
  size_t start = 0;
  size_t pos   = 0;

  while ((pos = str.find_first_of(delimiters, start)) != std::string_view::npos)
  {
    if (keep_empty || pos > start)
    {
      result.emplace_back(str.substr(start, pos - start));
    }
    start = pos + 1;
  }

  if (keep_empty || start < str.size())
  {
    result.emplace_back(str.substr(start));
  }

  return result;
}

std::vector<std::string> split_lines(std::string_view str, bool keep_empty)
{
  std::vector<std::string> result;
  size_t start = 0;
  size_t pos   = 0;

  while (pos < str.size())
  {
    // Find next line ending
    size_t line_end = str.find_first_of("\r\n", pos);
    if (line_end == std::string_view::npos)
    {
      line_end = str.size();
    }

    if (keep_empty || line_end > start)
    {
      result.emplace_back(str.substr(start, line_end - start));
    }

    // Skip line ending(s)
    if (line_end < str.size())
    {
      if (str[line_end] == '\r' && line_end + 1 < str.size() &&
          str[line_end + 1] == '\n')
      {
        pos = line_end + 2;
      }
      else
      {
        pos = line_end + 1;
      }
    }
    else
    {
      pos = line_end;
    }
    start = pos;
  }

  // Handle trailing empty line
  if (keep_empty && start == str.size() && !str.empty() &&
      (str.back() == '\n' || str.back() == '\r'))
  {
    result.emplace_back();
  }

  return result;
}

std::vector<std::string> split_whitespace(std::string_view str)
{
  std::vector<std::string> result;
  size_t start = 0;
  size_t pos   = 0;

  while (pos < str.size())
  {
    // Skip whitespace
    while (pos < str.size() && is_whitespace(str[pos]))
    {
      ++pos;
    }
    start = pos;

    // Find end of token
    while (pos < str.size() && !is_whitespace(str[pos]))
    {
      ++pos;
    }

    if (pos > start)
    {
      result.emplace_back(str.substr(start, pos - start));
    }
  }

  return result;
}

std::vector<std::string> split_n(std::string_view str,
                                 char delimiter,
                                 size_t max_parts)
{
  if (max_parts == 0)
  {
    return split(str, delimiter, true);
  }

  std::vector<std::string> result;
  result.reserve(max_parts);
  size_t start = 0;
  size_t pos   = 0;

  while ((pos = str.find(delimiter, start)) != std::string_view::npos &&
         result.size() < max_parts - 1)
  {
    result.emplace_back(str.substr(start, pos - start));
    start = pos + 1;
  }

  result.emplace_back(str.substr(start));
  return result;
}

std::string join(const std::vector<std::string>& parts,
                 std::string_view delimiter)
{
  if (parts.empty())
  {
    return std::string();
  }

  size_t total_size = 0;
  for (const auto& part : parts)
  {
    total_size += part.size();
  }
  total_size += delimiter.size() * (parts.size() - 1);

  std::string result;
  result.reserve(total_size);

  result.append(parts[0]);
  for (size_t i = 1; i < parts.size(); ++i)
  {
    result.append(delimiter);
    result.append(parts[i]);
  }

  return result;
}

std::string join(const std::vector<std::string>& parts, char delimiter)
{
  return join(parts, std::string_view(&delimiter, 1));
}

// ============================================================================
// Substring Extraction
// ============================================================================

std::string left(std::string_view str, size_t n)
{
  return std::string(str.substr(0, std::min(n, str.size())));
}

std::string right(std::string_view str, size_t n)
{
  if (n >= str.size())
  {
    return std::string(str);
  }
  return std::string(str.substr(str.size() - n));
}

std::string mid(std::string_view str, size_t start, size_t length)
{
  if (start >= str.size())
  {
    return std::string();
  }
  return std::string(str.substr(start, length));
}

std::string slice(std::string_view str, int start, int end)
{
  int len = static_cast<int>(str.size());

  // Handle negative indices
  if (start < 0)
  {
    start = std::max(0, len + start);
  }
  if (end == std::numeric_limits<int>::max())
  {
    end = len;
  }
  else if (end < 0)
  {
    end = std::max(0, len + end);
  }

  // Clamp to valid range
  start = std::min(start, len);
  end   = std::min(end, len);

  if (start >= end)
  {
    return std::string();
  }

  return std::string(str.substr(static_cast<size_t>(start),
                                static_cast<size_t>(end - start)));
}

std::string truncate(std::string_view str,
                     size_t max_length,
                     std::string_view ellipsis)
{
  if (str.size() <= max_length)
  {
    return std::string(str);
  }
  if (max_length <= ellipsis.size())
  {
    return std::string(ellipsis.substr(0, max_length));
  }
  std::string result;
  result.reserve(max_length);
  result.append(str.substr(0, max_length - ellipsis.size()));
  result.append(ellipsis);
  return result;
}

// ============================================================================
// Padding and Alignment
// ============================================================================

std::string pad_left(std::string_view str, size_t width, char fill)
{
  if (str.size() >= width)
  {
    return std::string(str);
  }
  std::string result(width - str.size(), fill);
  result.append(str);
  return result;
}

std::string pad_right(std::string_view str, size_t width, char fill)
{
  if (str.size() >= width)
  {
    return std::string(str);
  }
  std::string result(str);
  result.append(width - str.size(), fill);
  return result;
}

std::string center(std::string_view str, size_t width, char fill)
{
  if (str.size() >= width)
  {
    return std::string(str);
  }
  size_t padding    = width - str.size();
  size_t left_pad   = padding / 2;
  size_t right_pad  = padding - left_pad;
  std::string result;
  result.reserve(width);
  result.append(left_pad, fill);
  result.append(str);
  result.append(right_pad, fill);
  return result;
}

std::string repeat(std::string_view str, size_t n)
{
  if (n == 0 || str.empty())
  {
    return std::string();
  }
  std::string result;
  result.reserve(str.size() * n);
  for (size_t i = 0; i < n; ++i)
  {
    result.append(str);
  }
  return result;
}

// ============================================================================
// Validation and Type Checking
// ============================================================================

bool is_empty(std::string_view str)
{
  return str.empty();
}

bool is_blank(std::string_view str)
{
  return std::all_of(str.begin(), str.end(), is_whitespace);
}

bool is_numeric(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }
  return std::all_of(str.begin(), str.end(), is_ascii_digit);
}

bool is_integer(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }
  size_t start = 0;
  if (str[0] == '+' || str[0] == '-')
  {
    start = 1;
    if (str.size() == 1)
    {
      return false;
    }
  }
  return std::all_of(str.begin() + static_cast<long>(start),
                     str.end(),
                     is_ascii_digit);
}

bool is_float(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }

  size_t pos = 0;

  // Optional sign
  if (str[pos] == '+' || str[pos] == '-')
  {
    ++pos;
  }

  if (pos >= str.size())
  {
    return false;
  }

  bool has_digits = false;

  // Integer part
  while (pos < str.size() && is_ascii_digit(str[pos]))
  {
    has_digits = true;
    ++pos;
  }

  // Decimal part
  if (pos < str.size() && str[pos] == '.')
  {
    ++pos;
    while (pos < str.size() && is_ascii_digit(str[pos]))
    {
      has_digits = true;
      ++pos;
    }
  }

  // Must have at least some digits
  if (!has_digits)
  {
    return false;
  }

  // Exponent part
  if (pos < str.size() && (str[pos] == 'e' || str[pos] == 'E'))
  {
    ++pos;
    if (pos < str.size() && (str[pos] == '+' || str[pos] == '-'))
    {
      ++pos;
    }
    if (pos >= str.size() || !is_ascii_digit(str[pos]))
    {
      return false;
    }
    while (pos < str.size() && is_ascii_digit(str[pos]))
    {
      ++pos;
    }
  }

  return pos == str.size();
}

bool is_alpha(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }
  return std::all_of(str.begin(), str.end(), is_ascii_alpha);
}

bool is_alphanumeric(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }
  return std::all_of(str.begin(), str.end(), [](char ch) {
    return is_ascii_alpha(ch) || is_ascii_digit(ch);
  });
}

bool is_hex(std::string_view str, bool allow_prefix)
{
  if (str.empty())
  {
    return false;
  }
  size_t start = 0;
  if (allow_prefix && str.size() >= 2 && str[0] == '0' &&
      (str[1] == 'x' || str[1] == 'X'))
  {
    start = 2;
    if (str.size() == 2)
    {
      return false;
    }
  }
  return std::all_of(str.begin() + static_cast<long>(start),
                     str.end(),
                     is_hex_digit);
}

bool is_binary(std::string_view str, bool allow_prefix)
{
  if (str.empty())
  {
    return false;
  }
  size_t start = 0;
  if (allow_prefix && str.size() >= 2 && str[0] == '0' &&
      (str[1] == 'b' || str[1] == 'B'))
  {
    start = 2;
    if (str.size() == 2)
    {
      return false;
    }
  }
  return std::all_of(str.begin() + static_cast<long>(start),
                     str.end(),
                     [](char ch) { return ch == '0' || ch == '1'; });
}

bool is_octal(std::string_view str, bool allow_prefix)
{
  if (str.empty())
  {
    return false;
  }
  size_t start = 0;
  if (allow_prefix)
  {
    if (str.size() >= 2 && str[0] == '0' && (str[1] == 'o' || str[1] == 'O'))
    {
      start = 2;
      if (str.size() == 2)
      {
        return false;
      }
    }
    else if (str[0] == '0' && str.size() > 1)
    {
      start = 1;
    }
  }
  return std::all_of(str.begin() + static_cast<long>(start),
                     str.end(),
                     [](char ch) { return ch >= '0' && ch <= '7'; });
}

bool is_ascii(std::string_view str)
{
  return std::all_of(str.begin(), str.end(), [](char ch) {
    return static_cast<unsigned char>(ch) < 128;
  });
}

bool is_printable(std::string_view str)
{
  return std::all_of(str.begin(), str.end(), [](char ch) {
    return ch >= 32 && ch <= 126;
  });
}

bool is_upper(std::string_view str)
{
  bool has_letters = false;
  for (char ch : str)
  {
    if (is_ascii_lower(ch))
    {
      return false;
    }
    if (is_ascii_upper(ch))
    {
      has_letters = true;
    }
  }
  return has_letters;
}

bool is_lower(std::string_view str)
{
  bool has_letters = false;
  for (char ch : str)
  {
    if (is_ascii_upper(ch))
    {
      return false;
    }
    if (is_ascii_lower(ch))
    {
      has_letters = true;
    }
  }
  return has_letters;
}

bool is_identifier(std::string_view str)
{
  if (str.empty())
  {
    return false;
  }
  if (!is_ascii_alpha(str[0]) && str[0] != '_')
  {
    return false;
  }
  return std::all_of(str.begin() + 1, str.end(), [](char ch) {
    return is_ascii_alpha(ch) || is_ascii_digit(ch) || ch == '_';
  });
}

// ============================================================================
// Number Parsing
// ============================================================================

std::optional<int> to_int(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  // Trim whitespace
  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Skip '+' sign (std::from_chars doesn't accept it)
  if (trimmed.front() == '+')
  {
    trimmed.remove_prefix(1);
    if (trimmed.empty())
    {
      return std::nullopt;
    }
  }

  int result = 0;
  auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), result);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<int> to_int(std::string_view str, int base)
{
  // base == 0 means auto-detect, otherwise must be in range [2, 36]
  if (str.empty() || base < 0 || base > 36 || (base != 0 && base < 2))
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Handle auto-detect (base 0)
  if (base == 0)
  {
    if (trimmed.size() >= 2 && trimmed[0] == '0')
    {
      if (trimmed[1] == 'x' || trimmed[1] == 'X')
      {
        base    = 16;
        trimmed = trimmed.substr(2);
      }
      else if (trimmed[1] == 'b' || trimmed[1] == 'B')
      {
        base    = 2;
        trimmed = trimmed.substr(2);
      }
      else if (trimmed[1] == 'o' || trimmed[1] == 'O')
      {
        base    = 8;
        trimmed = trimmed.substr(2);
      }
      else
      {
        base = 10;
      }
    }
    else
    {
      base = 10;
    }
  }
  else
  {
    // Skip prefix for explicit bases
    if (base == 16 && trimmed.size() >= 2 && trimmed[0] == '0' &&
        (trimmed[1] == 'x' || trimmed[1] == 'X'))
    {
      trimmed = trimmed.substr(2);
    }
    else if (base == 2 && trimmed.size() >= 2 && trimmed[0] == '0' &&
             (trimmed[1] == 'b' || trimmed[1] == 'B'))
    {
      trimmed = trimmed.substr(2);
    }
    else if (base == 8 && trimmed.size() >= 2 && trimmed[0] == '0' &&
             (trimmed[1] == 'o' || trimmed[1] == 'O'))
    {
      trimmed = trimmed.substr(2);
    }
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  int result = 0;
  auto [ptr, ec] = std::from_chars(trimmed.data(),
                                   trimmed.data() + trimmed.size(),
                                   result,
                                   base);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<long> to_long(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  long result = 0;
  auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), result);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<long long> to_llong(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  long long result = 0;
  auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), result);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<unsigned long> to_ulong(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Reject negative numbers
  if (trimmed[0] == '-')
  {
    return std::nullopt;
  }

  unsigned long result = 0;
  auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), result);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<unsigned long long> to_ullong(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Reject negative numbers
  if (trimmed[0] == '-')
  {
    return std::nullopt;
  }

  unsigned long long result = 0;
  auto [ptr, ec] =
      std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), result);

  if (ec == std::errc{} && ptr == trimmed.data() + trimmed.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<float> to_float(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  // Use std::from_chars if available for floats (C++17 doesn't guarantee it)
  // Fall back to strtof for compatibility
  std::string temp_str(trimmed);
  char* end       = nullptr;
  float result    = std::strtof(temp_str.c_str(), &end);

  if (end == temp_str.c_str() + temp_str.size())
  {
    return result;
  }
  return std::nullopt;
}

std::optional<double> to_double(std::string_view str)
{
  if (str.empty())
  {
    return std::nullopt;
  }

  std::string_view trimmed = str;
  while (!trimmed.empty() && is_whitespace(trimmed.front()))
  {
    trimmed.remove_prefix(1);
  }
  while (!trimmed.empty() && is_whitespace(trimmed.back()))
  {
    trimmed.remove_suffix(1);
  }

  if (trimmed.empty())
  {
    return std::nullopt;
  }

  std::string temp_str(trimmed);
  char* end       = nullptr;
  double result   = std::strtod(temp_str.c_str(), &end);

  if (end == temp_str.c_str() + temp_str.size())
  {
    return result;
  }
  return std::nullopt;
}

// ============================================================================
// Number to String Conversion
// ============================================================================

std::string from_int(int value)
{
  return std::to_string(value);
}

std::string from_int(int value, int base, bool uppercase)
{
  if (base < 2 || base > 36)
  {
    return std::string();
  }

  if (value == 0)
  {
    return "0";
  }

  bool negative = value < 0;
  unsigned int abs_value =
      negative ? static_cast<unsigned int>(-static_cast<long>(value))
               : static_cast<unsigned int>(value);

  std::string result;
  while (abs_value > 0)
  {
    int digit = static_cast<int>(abs_value % static_cast<unsigned int>(base));
    if (digit < 10)
    {
      result.push_back(static_cast<char>('0' + digit));
    }
    else
    {
      result.push_back(
          static_cast<char>((uppercase ? 'A' : 'a') + digit - 10));
    }
    abs_value /= static_cast<unsigned int>(base);
  }

  if (negative)
  {
    result.push_back('-');
  }

  std::reverse(result.begin(), result.end());
  return result;
}

std::string from_long(long value)
{
  return std::to_string(value);
}

std::string from_double(double value, int precision)
{
  std::ostringstream oss;
  oss << std::setprecision(precision) << value;
  return oss.str();
}

std::string from_double_fixed(double value, int precision)
{
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

std::string from_double_scientific(double value, int precision)
{
  std::ostringstream oss;
  oss << std::scientific << std::setprecision(precision) << value;
  return oss.str();
}

// ============================================================================
// Encoding and Decoding
// ============================================================================

std::string to_hex(std::string_view data, bool uppercase)
{
  std::string result;
  result.reserve(data.size() * 2);
  for (unsigned char ch : data)
  {
    result.push_back(hex_char(ch >> 4, uppercase));
    result.push_back(hex_char(ch & 0x0F, uppercase));
  }
  return result;
}

std::optional<std::string> from_hex(std::string_view hex)
{
  std::string_view input = hex;

  // Skip 0x prefix
  if (input.size() >= 2 && input[0] == '0' &&
      (input[1] == 'x' || input[1] == 'X'))
  {
    input = input.substr(2);
  }

  if (input.size() % 2 != 0)
  {
    return std::nullopt;
  }

  std::string result;
  result.reserve(input.size() / 2);

  for (size_t i = 0; i < input.size(); i += 2)
  {
    int high = hex_value(input[i]);
    int low  = hex_value(input[i + 1]);
    if (high < 0 || low < 0)
    {
      return std::nullopt;
    }
    result.push_back(static_cast<char>((high << 4) | low));
  }

  return result;
}

std::string to_base64(std::string_view data)
{
  std::string result;
  result.reserve((data.size() + 2) / 3 * 4);

  size_t i = 0;
  while (i + 2 < data.size())
  {
    uint32_t triple =
        (static_cast<unsigned char>(data[i]) << 16) |
        (static_cast<unsigned char>(data[i + 1]) << 8) |
        static_cast<unsigned char>(data[i + 2]);

    result.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
    result.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);
    result.push_back(BASE64_CHARS[(triple >> 6) & 0x3F]);
    result.push_back(BASE64_CHARS[triple & 0x3F]);
    i += 3;
  }

  if (i < data.size())
  {
    uint32_t triple = static_cast<unsigned char>(data[i]) << 16;
    if (i + 1 < data.size())
    {
      triple |= static_cast<unsigned char>(data[i + 1]) << 8;
    }

    result.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
    result.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);

    if (i + 1 < data.size())
    {
      result.push_back(BASE64_CHARS[(triple >> 6) & 0x3F]);
    }
    else
    {
      result.push_back('=');
    }
    result.push_back('=');
  }

  return result;
}

std::optional<std::string> from_base64(std::string_view base64)
{
  if (base64.empty())
  {
    return std::string();
  }

  // Remove whitespace and validate length
  std::string clean;
  clean.reserve(base64.size());
  for (char ch : base64)
  {
    if (!is_whitespace(ch))
    {
      clean.push_back(ch);
    }
  }

  if (clean.size() % 4 != 0)
  {
    return std::nullopt;
  }

  std::string result;
  result.reserve(clean.size() / 4 * 3);

  for (size_t i = 0; i < clean.size(); i += 4)
  {
    int v0 = base64_index(clean[i]);
    int v1 = base64_index(clean[i + 1]);
    int v2 = clean[i + 2] == '=' ? 0 : base64_index(clean[i + 2]);
    int v3 = clean[i + 3] == '=' ? 0 : base64_index(clean[i + 3]);

    if (v0 < 0 || v1 < 0 || (clean[i + 2] != '=' && v2 < 0) ||
        (clean[i + 3] != '=' && v3 < 0))
    {
      return std::nullopt;
    }

    uint32_t triple = (static_cast<uint32_t>(v0) << 18) |
                      (static_cast<uint32_t>(v1) << 12) |
                      (static_cast<uint32_t>(v2) << 6) |
                      static_cast<uint32_t>(v3);

    result.push_back(static_cast<char>((triple >> 16) & 0xFF));
    if (clean[i + 2] != '=')
    {
      result.push_back(static_cast<char>((triple >> 8) & 0xFF));
    }
    if (clean[i + 3] != '=')
    {
      result.push_back(static_cast<char>(triple & 0xFF));
    }
  }

  return result;
}

std::string url_encode(std::string_view str)
{
  std::string result;
  result.reserve(str.size() * 3); // Worst case

  for (unsigned char ch : str)
  {
    if (is_ascii_alpha(static_cast<char>(ch)) || is_ascii_digit(static_cast<char>(ch)) ||
        ch == '-' || ch == '_' || ch == '.' || ch == '~')
    {
      result.push_back(static_cast<char>(ch));
    }
    else
    {
      result.push_back('%');
      result.push_back(hex_char(ch >> 4, true));
      result.push_back(hex_char(ch & 0x0F, true));
    }
  }

  return result;
}

std::optional<std::string> url_decode(std::string_view str)
{
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '%')
    {
      if (i + 2 >= str.size())
      {
        return std::nullopt;
      }
      int high = hex_value(str[i + 1]);
      int low  = hex_value(str[i + 2]);
      if (high < 0 || low < 0)
      {
        return std::nullopt;
      }
      result.push_back(static_cast<char>((high << 4) | low));
      i += 2;
    }
    else if (str[i] == '+')
    {
      result.push_back(' ');
    }
    else
    {
      result.push_back(str[i]);
    }
  }

  return result;
}

std::string html_escape(std::string_view str)
{
  std::string result;
  result.reserve(str.size() * 2);

  for (char ch : str)
  {
    switch (ch)
    {
    case '&':
      result.append("&amp;");
      break;
    case '<':
      result.append("&lt;");
      break;
    case '>':
      result.append("&gt;");
      break;
    case '"':
      result.append("&quot;");
      break;
    case '\'':
      result.append("&#39;");
      break;
    default:
      result.push_back(ch);
      break;
    }
  }

  return result;
}

std::string html_unescape(std::string_view str)
{
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '&')
    {
      if (str.substr(i, 4) == "&lt;")
      {
        result.push_back('<');
        i += 3;
      }
      else if (str.substr(i, 4) == "&gt;")
      {
        result.push_back('>');
        i += 3;
      }
      else if (str.substr(i, 5) == "&amp;")
      {
        result.push_back('&');
        i += 4;
      }
      else if (str.substr(i, 6) == "&quot;")
      {
        result.push_back('"');
        i += 5;
      }
      else if (str.substr(i, 6) == "&apos;")
      {
        result.push_back('\'');
        i += 5;
      }
      else if (str.substr(i, 5) == "&#39;")
      {
        result.push_back('\'');
        i += 4;
      }
      else
      {
        result.push_back(str[i]);
      }
    }
    else
    {
      result.push_back(str[i]);
    }
  }

  return result;
}

std::string json_escape(std::string_view str)
{
  std::string result;
  result.reserve(str.size() * 2);

  for (unsigned char ch : str)
  {
    switch (ch)
    {
    case '"':
      result.append("\\\"");
      break;
    case '\\':
      result.append("\\\\");
      break;
    case '\b':
      result.append("\\b");
      break;
    case '\f':
      result.append("\\f");
      break;
    case '\n':
      result.append("\\n");
      break;
    case '\r':
      result.append("\\r");
      break;
    case '\t':
      result.append("\\t");
      break;
    default:
      if (ch < 0x20)
      {
        result.append("\\u00");
        result.push_back(hex_char(ch >> 4, false));
        result.push_back(hex_char(ch & 0x0F, false));
      }
      else
      {
        result.push_back(static_cast<char>(ch));
      }
      break;
    }
  }

  return result;
}

std::optional<std::string> json_unescape(std::string_view str)
{
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '\\' && i + 1 < str.size())
    {
      switch (str[i + 1])
      {
      case '"':
        result.push_back('"');
        ++i;
        break;
      case '\\':
        result.push_back('\\');
        ++i;
        break;
      case '/':
        result.push_back('/');
        ++i;
        break;
      case 'b':
        result.push_back('\b');
        ++i;
        break;
      case 'f':
        result.push_back('\f');
        ++i;
        break;
      case 'n':
        result.push_back('\n');
        ++i;
        break;
      case 'r':
        result.push_back('\r');
        ++i;
        break;
      case 't':
        result.push_back('\t');
        ++i;
        break;
      case 'u':
        if (i + 5 < str.size())
        {
          int h1 = hex_value(str[i + 2]);
          int h2 = hex_value(str[i + 3]);
          int h3 = hex_value(str[i + 4]);
          int h4 = hex_value(str[i + 5]);
          if (h1 < 0 || h2 < 0 || h3 < 0 || h4 < 0)
          {
            return std::nullopt;
          }
          int codepoint = (h1 << 12) | (h2 << 8) | (h3 << 4) | h4;
          if (codepoint < 0x80)
          {
            result.push_back(static_cast<char>(codepoint));
          }
          else if (codepoint < 0x800)
          {
            result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
          else
          {
            result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            result.push_back(
                static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
          }
          i += 5;
        }
        else
        {
          return std::nullopt;
        }
        break;
      default:
        return std::nullopt;
      }
    }
    else
    {
      result.push_back(str[i]);
    }
  }

  return result;
}

// ============================================================================
// Comparison
// ============================================================================

int compare_ignore_case(std::string_view a, std::string_view b)
{
  size_t min_len = std::min(a.size(), b.size());
  for (size_t i = 0; i < min_len; ++i)
  {
    char ca = to_ascii_lower(a[i]);
    char cb = to_ascii_lower(b[i]);
    if (ca != cb)
    {
      return ca < cb ? -1 : 1;
    }
  }
  if (a.size() != b.size())
  {
    return a.size() < b.size() ? -1 : 1;
  }
  return 0;
}

bool equals_ignore_case(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
  {
    return false;
  }
  for (size_t i = 0; i < a.size(); ++i)
  {
    if (to_ascii_lower(a[i]) != to_ascii_lower(b[i]))
    {
      return false;
    }
  }
  return true;
}

int natural_compare(std::string_view a, std::string_view b)
{
  size_t i = 0;
  size_t j = 0;

  while (i < a.size() && j < b.size())
  {
    char ca = a[i];
    char cb = b[j];

    // Both are digits - compare numerically
    if (is_ascii_digit(ca) && is_ascii_digit(cb))
    {
      // Skip leading zeros
      while (i < a.size() && a[i] == '0')
      {
        ++i;
      }
      while (j < b.size() && b[j] == '0')
      {
        ++j;
      }

      // Find end of number
      size_t ni = i;
      size_t nj = j;
      while (ni < a.size() && is_ascii_digit(a[ni]))
      {
        ++ni;
      }
      while (nj < b.size() && is_ascii_digit(b[nj]))
      {
        ++nj;
      }

      size_t len_a = ni - i;
      size_t len_b = nj - j;

      // Compare by length first
      if (len_a != len_b)
      {
        return len_a < len_b ? -1 : 1;
      }

      // Same length, compare digit by digit
      while (i < ni)
      {
        if (a[i] != b[j])
        {
          return a[i] < b[j] ? -1 : 1;
        }
        ++i;
        ++j;
      }
    }
    else
    {
      // Regular character comparison
      if (ca != cb)
      {
        return ca < cb ? -1 : 1;
      }
      ++i;
      ++j;
    }
  }

  // One string is prefix of the other
  if (i < a.size())
  {
    return 1;
  }
  if (j < b.size())
  {
    return -1;
  }
  return 0;
}

// ============================================================================
// Miscellaneous Utilities
// ============================================================================

std::string reverse(std::string_view str)
{
  std::string result(str);
  std::reverse(result.begin(), result.end());
  return result;
}

std::string squeeze(std::string_view str)
{
  if (str.empty())
  {
    return std::string();
  }
  std::string result;
  result.reserve(str.size());
  result.push_back(str[0]);
  for (size_t i = 1; i < str.size(); ++i)
  {
    if (str[i] != str[i - 1])
    {
      result.push_back(str[i]);
    }
  }
  return result;
}

std::string squeeze(std::string_view str, char ch)
{
  if (str.empty())
  {
    return std::string();
  }
  std::string result;
  result.reserve(str.size());
  result.push_back(str[0]);
  for (size_t i = 1; i < str.size(); ++i)
  {
    if (str[i] != ch || str[i - 1] != ch)
    {
      result.push_back(str[i]);
    }
  }
  return result;
}

std::string normalize_whitespace(std::string_view str)
{
  std::string result;
  result.reserve(str.size());
  bool in_whitespace = false;
  bool started       = false;

  for (char ch : str)
  {
    if (is_whitespace(ch))
    {
      if (started && !in_whitespace)
      {
        result.push_back(' ');
        in_whitespace = true;
      }
    }
    else
    {
      result.push_back(ch);
      in_whitespace = false;
      started       = true;
    }
  }

  // Remove trailing space
  if (!result.empty() && result.back() == ' ')
  {
    result.pop_back();
  }

  return result;
}

std::string word_wrap(std::string_view str, size_t width, bool break_words)
{
  if (width == 0)
  {
    return std::string(str);
  }

  std::string result;
  result.reserve(str.size() + str.size() / width);

  size_t line_start = 0;
  size_t last_space = std::string::npos;
  size_t pos        = 0;
  size_t col        = 0;

  while (pos < str.size())
  {
    char ch = str[pos];

    if (ch == '\n')
    {
      result.append(str.substr(line_start, pos - line_start + 1));
      line_start = pos + 1;
      last_space = std::string::npos;
      col        = 0;
    }
    else if (is_whitespace(ch))
    {
      last_space = pos;
      ++col;
    }
    else
    {
      ++col;
    }

    if (col > width)
    {
      if (last_space != std::string::npos && last_space > line_start)
      {
        result.append(str.substr(line_start, last_space - line_start));
        result.push_back('\n');
        line_start = last_space + 1;
        col        = pos - last_space;
        last_space = std::string::npos;
      }
      else if (break_words)
      {
        result.append(str.substr(line_start, pos - line_start));
        result.push_back('\n');
        line_start = pos;
        col        = 1;
        last_space = std::string::npos;
      }
    }

    ++pos;
  }

  if (line_start < str.size())
  {
    result.append(str.substr(line_start));
  }

  return result;
}

std::string indent(std::string_view str, std::string_view indent_str)
{
  std::string result;
  result.reserve(str.size() + str.size() / 20 * indent_str.size());

  result.append(indent_str);
  for (char ch : str)
  {
    result.push_back(ch);
    if (ch == '\n')
    {
      result.append(indent_str);
    }
  }

  // Remove trailing indent if string ends with newline
  if (!str.empty() && str.back() == '\n' &&
      result.size() >= indent_str.size())
  {
    result.resize(result.size() - indent_str.size());
  }

  return result;
}

std::string dedent(std::string_view str)
{
  if (str.empty())
  {
    return std::string();
  }

  // Find minimum indentation
  size_t min_indent   = std::string::npos;
  bool at_line_start  = true;
  size_t indent_count = 0;

  for (size_t i = 0; i < str.size(); ++i)
  {
    char ch = str[i];
    if (at_line_start)
    {
      if (ch == ' ' || ch == '\t')
      {
        ++indent_count;
      }
      else if (ch == '\n')
      {
        indent_count = 0; // Empty line, don't count
      }
      else
      {
        if (indent_count < min_indent)
        {
          min_indent = indent_count;
        }
        at_line_start = false;
      }
    }
    if (ch == '\n')
    {
      at_line_start = true;
      indent_count  = 0;
    }
  }

  if (min_indent == std::string::npos || min_indent == 0)
  {
    return std::string(str);
  }

  // Remove minimum indentation from each line
  std::string result;
  result.reserve(str.size());
  at_line_start = true;
  indent_count  = 0;

  for (char ch : str)
  {
    if (at_line_start && indent_count < min_indent)
    {
      if (ch == ' ' || ch == '\t')
      {
        ++indent_count;
        continue;
      }
    }
    result.push_back(ch);
    if (ch == '\n')
    {
      at_line_start = true;
      indent_count  = 0;
    }
    else
    {
      at_line_start = false;
    }
  }

  return result;
}

std::string common_prefix(std::string_view a, std::string_view b)
{
  size_t len = std::min(a.size(), b.size());
  size_t i   = 0;
  while (i < len && a[i] == b[i])
  {
    ++i;
  }
  return std::string(a.substr(0, i));
}

std::string common_suffix(std::string_view a, std::string_view b)
{
  size_t len = std::min(a.size(), b.size());
  size_t i   = 0;
  while (i < len && a[a.size() - 1 - i] == b[b.size() - 1 - i])
  {
    ++i;
  }
  return std::string(a.substr(a.size() - i));
}

size_t levenshtein_distance(std::string_view a, std::string_view b)
{
  if (a.empty())
  {
    return b.size();
  }
  if (b.empty())
  {
    return a.size();
  }

  std::vector<size_t> prev(b.size() + 1);
  std::vector<size_t> curr(b.size() + 1);

  for (size_t j = 0; j <= b.size(); ++j)
  {
    prev[j] = j;
  }

  for (size_t i = 1; i <= a.size(); ++i)
  {
    curr[0] = i;
    for (size_t j = 1; j <= b.size(); ++j)
    {
      size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
    }
    std::swap(prev, curr);
  }

  return prev[b.size()];
}

// ============================================================================
// Case Style Conversions
// ============================================================================

namespace
{

// Helper to split string into words for case conversion
std::vector<std::string> split_into_words(std::string_view str)
{
  std::vector<std::string> words;
  std::string              current_word;

  for (size_t i = 0; i < str.size(); ++i)
  {
    char ch = str[i];

    // Separators: space, underscore, hyphen
    if (ch == ' ' || ch == '_' || ch == '-')
    {
      if (!current_word.empty())
      {
        words.push_back(std::move(current_word));
        current_word.clear();
      }
      continue;
    }

    // Check for camelCase/PascalCase boundary
    if (is_ascii_upper(ch))
    {
      // If we have accumulated lowercase letters, this is a word boundary
      if (!current_word.empty() && is_ascii_lower(current_word.back()))
      {
        words.push_back(std::move(current_word));
        current_word.clear();
      }
      // Check for consecutive uppercase followed by lowercase (e.g., HTTPServer -> HTTP, Server)
      else if (!current_word.empty() && is_ascii_upper(current_word.back()) && i + 1 < str.size() &&
               is_ascii_lower(str[i + 1]))
      {
        words.push_back(std::move(current_word));
        current_word.clear();
      }
    }

    current_word.push_back(ch);
  }

  if (!current_word.empty())
  {
    words.push_back(std::move(current_word));
  }

  return words;
}

} // namespace

std::string to_snake_case(std::string_view str)
{
  auto words = split_into_words(str);
  if (words.empty())
  {
    return std::string();
  }

  std::string result;
  for (size_t i = 0; i < words.size(); ++i)
  {
    if (i > 0)
    {
      result.push_back('_');
    }
    for (char ch : words[i])
    {
      result.push_back(to_ascii_lower(ch));
    }
  }
  return result;
}

std::string to_camel_case(std::string_view str)
{
  auto words = split_into_words(str);
  if (words.empty())
  {
    return std::string();
  }

  std::string result;
  for (size_t i = 0; i < words.size(); ++i)
  {
    bool first_char = true;
    for (char ch : words[i])
    {
      if (first_char)
      {
        result.push_back(i == 0 ? to_ascii_lower(ch) : to_ascii_upper(ch));
        first_char = false;
      }
      else
      {
        result.push_back(to_ascii_lower(ch));
      }
    }
  }
  return result;
}

std::string to_pascal_case(std::string_view str)
{
  auto words = split_into_words(str);
  if (words.empty())
  {
    return std::string();
  }

  std::string result;
  for (const auto& word : words)
  {
    bool first_char = true;
    for (char ch : word)
    {
      if (first_char)
      {
        result.push_back(to_ascii_upper(ch));
        first_char = false;
      }
      else
      {
        result.push_back(to_ascii_lower(ch));
      }
    }
  }
  return result;
}

std::string to_kebab_case(std::string_view str)
{
  auto words = split_into_words(str);
  if (words.empty())
  {
    return std::string();
  }

  std::string result;
  for (size_t i = 0; i < words.size(); ++i)
  {
    if (i > 0)
    {
      result.push_back('-');
    }
    for (char ch : words[i])
    {
      result.push_back(to_ascii_lower(ch));
    }
  }
  return result;
}

std::string to_screaming_snake_case(std::string_view str)
{
  auto words = split_into_words(str);
  if (words.empty())
  {
    return std::string();
  }

  std::string result;
  for (size_t i = 0; i < words.size(); ++i)
  {
    if (i > 0)
    {
      result.push_back('_');
    }
    for (char ch : words[i])
    {
      result.push_back(to_ascii_upper(ch));
    }
  }
  return result;
}

// ============================================================================
// Pattern Matching
// ============================================================================

bool matches_pattern(std::string_view str,
                     std::string_view pattern,
                     bool case_sensitive)
{
  size_t s = 0; // String position
  size_t p = 0; // Pattern position
  size_t star_p   = std::string::npos; // Last '*' position in pattern
  size_t star_s   = std::string::npos; // String position when we saw '*'

  while (s < str.size())
  {
    if (p < pattern.size())
    {
      char pc = pattern[p];
      char sc = str[s];

      // Case-insensitive comparison
      if (!case_sensitive)
      {
        pc = to_ascii_lower(pc);
        sc = to_ascii_lower(sc);
      }

      if (pc == '?')
      {
        // '?' matches any single character
        ++s;
        ++p;
        continue;
      }

      if (pc == '*')
      {
        // '*' matches any sequence - remember position for backtracking
        star_p = p;
        star_s = s;
        ++p;
        continue;
      }

      if (pc == sc)
      {
        // Characters match
        ++s;
        ++p;
        continue;
      }
    }

    // No match - try backtracking to last '*'
    if (star_p != std::string::npos)
    {
      p = star_p + 1;
      ++star_s;
      s = star_s;
      continue;
    }

    // No '*' to backtrack to - pattern doesn't match
    return false;
  }

  // Skip trailing '*' in pattern
  while (p < pattern.size() && pattern[p] == '*')
  {
    ++p;
  }

  return p == pattern.size();
}

// ============================================================================
// Additional String Distance/Similarity Functions
// ============================================================================

size_t hamming_distance(std::string_view a, std::string_view b)
{
  if (a.size() != b.size())
  {
    return std::numeric_limits<size_t>::max();
  }

  size_t distance = 0;
  for (size_t i = 0; i < a.size(); ++i)
  {
    if (a[i] != b[i])
    {
      ++distance;
    }
  }
  return distance;
}

double similarity(std::string_view a, std::string_view b)
{
  if (a.empty() && b.empty())
  {
    return 1.0;
  }

  size_t max_len = std::max(a.size(), b.size());
  if (max_len == 0)
  {
    return 1.0;
  }

  size_t distance = levenshtein_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(max_len));
}

bool fuzzy_match(std::string_view str,
                 std::string_view target,
                 size_t max_distance)
{
  return levenshtein_distance(str, target) <= max_distance;
}

// ============================================================================
// Text Analysis Functions
// ============================================================================

size_t word_count(std::string_view str)
{
  size_t count        = 0;
  bool   in_word      = false;

  for (char ch : str)
  {
    if (is_whitespace(ch))
    {
      in_word = false;
    }
    else if (!in_word)
    {
      ++count;
      in_word = true;
    }
  }

  return count;
}

std::string expand_tabs(std::string_view str, size_t tab_width)
{
  if (tab_width == 0)
  {
    return remove_all(str, '\t');
  }

  std::string result;
  result.reserve(str.size());
  size_t col = 0;

  for (char ch : str)
  {
    if (ch == '\t')
    {
      size_t spaces = tab_width - (col % tab_width);
      result.append(spaces, ' ');
      col += spaces;
    }
    else if (ch == '\n')
    {
      result.push_back(ch);
      col = 0;
    }
    else
    {
      result.push_back(ch);
      ++col;
    }
  }

  return result;
}

std::string normalize_line_endings(std::string_view str, std::string_view ending)
{
  std::string result;
  result.reserve(str.size());

  for (size_t i = 0; i < str.size(); ++i)
  {
    if (str[i] == '\r')
    {
      result.append(ending);
      // Skip LF if CRLF
      if (i + 1 < str.size() && str[i + 1] == '\n')
      {
        ++i;
      }
    }
    else if (str[i] == '\n')
    {
      result.append(ending);
    }
    else
    {
      result.push_back(str[i]);
    }
  }

  return result;
}

bool is_palindrome(std::string_view str)
{
  // Extract only alphanumeric characters in lowercase
  std::string clean;
  clean.reserve(str.size());
  for (char ch : str)
  {
    if (is_ascii_alpha(ch) || is_ascii_digit(ch))
    {
      clean.push_back(to_ascii_lower(ch));
    }
  }

  if (clean.empty())
  {
    return true;
  }

  size_t left  = 0;
  size_t right = clean.size() - 1;
  while (left < right)
  {
    if (clean[left] != clean[right])
    {
      return false;
    }
    ++left;
    --right;
  }
  return true;
}

std::string quote(std::string_view str, char quote_char)
{
  std::string result;
  result.reserve(str.size() + 2);
  result.push_back(quote_char);
  result.append(str);
  result.push_back(quote_char);
  return result;
}

std::string unquote(std::string_view str)
{
  if (str.size() < 2)
  {
    return std::string(str);
  }

  char first = str.front();
  char last  = str.back();

  // Check for matching quotes
  if ((first == '"' && last == '"') ||
      (first == '\'' && last == '\'') ||
      (first == '`' && last == '`'))
  {
    return std::string(str.substr(1, str.size() - 2));
  }

  return std::string(str);
}

} // namespace fb
