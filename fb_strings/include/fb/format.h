#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <optional>

namespace fb
{

// ============================================================================
// Forward Declarations
// ============================================================================

template<typename T, typename Enable = void>
struct formatter;

// ============================================================================
// Format Specification
// ============================================================================

/**
 * @brief Parsed format specification from {:spec} syntax
 */
struct format_spec
{
  char fill      = ' ';   ///< Fill character
  char align     = '\0';  ///< Alignment: '<' left, '>' right, '^' center
  char sign      = '-';   ///< Sign: '+', '-', or ' '
  bool alt_form  = false; ///< Alternate form (#)
  bool zero_pad  = false; ///< Zero padding (0)
  int width      = 0;     ///< Minimum field width
  int precision  = -1;    ///< Precision for floats/strings
  char type      = '\0';  ///< Type specifier: d, x, X, o, b, B, e, E, f, F, g, G, s
};

/**
 * @brief Parse format specification string
 * @param spec Format specification (everything after the colon)
 * @return Parsed format_spec
 */
format_spec parse_format_spec(std::string_view spec);

// ============================================================================
// Formatter Specializations
// ============================================================================

/**
 * @brief Formatter for integral types
 */
template<typename T>
struct formatter<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(T value) const;
};

/**
 * @brief Formatter for bool
 */
template<>
struct formatter<bool>
{
  format_spec m_spec;
  bool m_as_string = true;

  void parse(std::string_view spec);
  std::string format(bool value) const;
};

/**
 * @brief Formatter for floating-point types
 */
template<typename T>
struct formatter<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(T value) const;
};

/**
 * @brief Formatter for const char*
 */
template<>
struct formatter<const char*>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(const char* value) const;
};

/**
 * @brief Formatter for char*
 */
template<>
struct formatter<char*>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(char* value) const;
};

/**
 * @brief Formatter for std::string
 */
template<>
struct formatter<std::string>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(const std::string& value) const;
};

/**
 * @brief Formatter for std::string_view
 */
template<>
struct formatter<std::string_view>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(std::string_view value) const;
};

/**
 * @brief Formatter for char
 */
template<>
struct formatter<char>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(char value) const;
};

/**
 * @brief Formatter for pointers
 */
template<typename T>
struct formatter<T*, std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, char>>>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(T* value) const;
};

/**
 * @brief Formatter for nullptr_t
 */
template<>
struct formatter<std::nullptr_t>
{
  format_spec m_spec;

  void parse(std::string_view spec);
  std::string format(std::nullptr_t value) const;
};

// ============================================================================
// Format Context and Argument Storage
// ============================================================================

namespace detail
{

/**
 * @brief Type-erased format argument
 */
class format_arg
{
public:
  format_arg() = default;

  template<typename T>
  explicit format_arg(const T& value);

  std::string format(const format_spec& spec) const;
  bool is_valid() const noexcept { return m_format_fn != nullptr; }

private:
  using format_fn = std::string (*)(const void*, const format_spec&);
  const void* m_value   = nullptr;
  format_fn m_format_fn = nullptr;
};

/**
 * @brief Storage for format arguments
 */
template<typename... Args>
class format_args_store
{
public:
  explicit format_args_store(const Args&... args);

  const format_arg& get(size_t index) const;
  size_t size() const noexcept { return sizeof...(Args); }

private:
  std::array<format_arg, sizeof...(Args)> m_args;
};

/**
 * @brief Type-erased format arguments view
 */
class format_args
{
public:
  template<typename... Args>
  format_args(const format_args_store<Args...>& store);

  const format_arg& get(size_t index) const;
  size_t size() const noexcept { return m_size; }

private:
  const format_arg* m_args = nullptr;
  size_t m_size            = 0;
};

/**
 * @brief Core format implementation
 */
std::string vformat(std::string_view fmt, format_args args);

} // namespace detail

// ============================================================================
// Public Format Functions
// ============================================================================

/**
 * @brief Format a string with arguments
 *
 * Supports:
 *   - {} for automatic argument indexing
 *   - {0}, {1}, etc. for positional arguments
 *   - {:spec} for format specifications
 *   - {0:spec} for positional with specification
 *
 * Format specification syntax:
 *   [[fill]align][sign][#][0][width][.precision][type]
 *
 *   fill    - Any character (default: space)
 *   align   - '<' left, '>' right, '^' center
 *   sign    - '+' always, '-' negative only (default), ' ' space for positive
 *   #       - Alternate form (0x for hex, etc.)
 *   0       - Zero-pad numeric values
 *   width   - Minimum field width
 *   .precision - Max string length or float precision
 *   type    - d decimal, x/X hex, o octal, b/B binary, e/E/f/F/g/G float, s string
 *
 * @tparam Args Argument types
 * @param fmt Format string
 * @param args Format arguments
 * @return Formatted string
 *
 * @throws std::invalid_argument if format string is invalid
 * @throws std::out_of_range if argument index is out of range
 *
 * @example
 *   format("Hello, {}!", "World")           -> "Hello, World!"
 *   format("{1} before {0}", "second", "first") -> "first before second"
 *   format("{:>10}", "hi")                  -> "        hi"
 *   format("{:05d}", 42)                    -> "00042"
 *   format("{:.2f}", 3.14159)               -> "3.14"
 *   format("{:#x}", 255)                    -> "0xff"
 */
template<typename... Args>
std::string format(std::string_view fmt, const Args&... args)
{
  detail::format_args_store<Args...> store(args...);
  return detail::vformat(fmt, detail::format_args(store));
}

/**
 * @brief Format a string with arguments (exception-free version)
 *
 * Same as format() but returns std::nullopt on error instead of throwing.
 *
 * @tparam Args Argument types
 * @param fmt Format string
 * @param args Format arguments
 * @return Formatted string or std::nullopt on error
 */
template<typename... Args>
std::optional<std::string> try_format(std::string_view fmt, const Args&... args)
{
  try
  {
    return format(fmt, args...);
  }
  catch (...)
  {
    return std::nullopt;
  }
}

// ============================================================================
// Implementation Details
// ============================================================================

namespace detail
{

// Helper to decay char arrays to const char*
template<typename T>
struct format_type_helper
{
  using type = T;
};

template<size_t N>
struct format_type_helper<char[N]>
{
  using type = const char*;
};

template<size_t N>
struct format_type_helper<const char[N]>
{
  using type = const char*;
};

template<typename T>
using format_type = typename format_type_helper<std::remove_cv_t<T>>::type;

template<typename T>
format_arg::format_arg(const T& value)
  : m_value(&value)
  , m_format_fn([](const void* ptr, const format_spec& spec) -> std::string {
      using actual_type = format_type<T>;
      formatter<actual_type> fmt;
      fmt.m_spec = spec;
      if constexpr (std::is_array_v<T>)
      {
        // For char arrays, treat as const char*
        return fmt.format(static_cast<const char*>(
            static_cast<const void*>(ptr)));
      }
      else
      {
        return fmt.format(*static_cast<const T*>(ptr));
      }
    })
{
}

template<typename... Args>
format_args_store<Args...>::format_args_store(const Args&... args)
  : m_args{format_arg(args)...}
{
}

template<typename... Args>
const format_arg& format_args_store<Args...>::get(size_t index) const
{
  if (index >= sizeof...(Args))
  {
    throw std::out_of_range("format argument index out of range");
  }
  return m_args[index];
}

template<typename... Args>
format_args::format_args(const format_args_store<Args...>& store)
  : m_args(store.size() > 0 ? &store.get(0) : nullptr)
  , m_size(store.size())
{
}

} // namespace detail

} // namespace fb
