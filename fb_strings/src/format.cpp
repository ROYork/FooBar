#include <fb/format.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fb
{

// ============================================================================
// Format Specification Parsing
// ============================================================================

format_spec parse_format_spec(std::string_view spec)
{
  format_spec result;

  if (spec.empty())
  {
    return result;
  }

  size_t pos = 0;

  // Check for fill and alignment
  // If the second character is an alignment character, the first is fill
  if (spec.size() >= 2 &&
      (spec[1] == '<' || spec[1] == '>' || spec[1] == '^'))
  {
    result.fill  = spec[0];
    result.align = spec[1];
    pos          = 2;
  }
  else if (!spec.empty() &&
           (spec[0] == '<' || spec[0] == '>' || spec[0] == '^'))
  {
    result.align = spec[0];
    pos          = 1;
  }

  // Sign
  if (pos < spec.size() &&
      (spec[pos] == '+' || spec[pos] == '-' || spec[pos] == ' '))
  {
    result.sign = spec[pos];
    ++pos;
  }

  // Alternate form (#)
  if (pos < spec.size() && spec[pos] == '#')
  {
    result.alt_form = true;
    ++pos;
  }

  // Zero padding
  if (pos < spec.size() && spec[pos] == '0')
  {
    result.zero_pad = true;
    ++pos;
  }

  // Width
  while (pos < spec.size() && spec[pos] >= '0' && spec[pos] <= '9')
  {
    result.width = result.width * 10 + (spec[pos] - '0');
    ++pos;
  }

  // Precision
  if (pos < spec.size() && spec[pos] == '.')
  {
    ++pos;
    result.precision = 0;
    while (pos < spec.size() && spec[pos] >= '0' && spec[pos] <= '9')
    {
      result.precision = result.precision * 10 + (spec[pos] - '0');
      ++pos;
    }
  }

  // Type
  if (pos < spec.size())
  {
    result.type = spec[pos];
  }

  return result;
}

// ============================================================================
// Helper Functions
// ============================================================================

namespace
{

std::string apply_alignment(std::string_view str,
                            const format_spec& spec)
{
  if (spec.width <= 0 || str.size() >= static_cast<size_t>(spec.width))
  {
    return std::string(str);
  }

  size_t padding = static_cast<size_t>(spec.width) - str.size();
  std::string result;
  result.reserve(static_cast<size_t>(spec.width));

  char align = spec.align;
  if (align == '\0')
  {
    align = '<'; // Default to left for strings
  }

  switch (align)
  {
  case '<': // Left align
    result.append(str);
    result.append(padding, spec.fill);
    break;
  case '>': // Right align
    result.append(padding, spec.fill);
    result.append(str);
    break;
  case '^': // Center align
  {
    size_t left_pad  = padding / 2;
    size_t right_pad = padding - left_pad;
    result.append(left_pad, spec.fill);
    result.append(str);
    result.append(right_pad, spec.fill);
    break;
  }
  }

  return result;
}

std::string format_integer_impl(long long value,
                                const format_spec& spec,
                                bool is_unsigned)
{
  std::string result;

  int base      = 10;
  bool uppercase = false;
  std::string prefix;

  switch (spec.type)
  {
  case 'x':
    base = 16;
    if (spec.alt_form && value != 0)
    {
      prefix = "0x";
    }
    break;
  case 'X':
    base      = 16;
    uppercase = true;
    if (spec.alt_form && value != 0)
    {
      prefix = "0X";
    }
    break;
  case 'o':
    base = 8;
    if (spec.alt_form && value != 0)
    {
      prefix = "0";
    }
    break;
  case 'b':
    base = 2;
    if (spec.alt_form && value != 0)
    {
      prefix = "0b";
    }
    break;
  case 'B':
    base      = 2;
    uppercase = true;
    if (spec.alt_form && value != 0)
    {
      prefix = "0B";
    }
    break;
  default:
    break;
  }

  // Handle sign
  std::string sign_str;
  if (!is_unsigned && value < 0)
  {
    sign_str = "-";
    value    = -value;
  }
  else if (spec.sign == '+')
  {
    sign_str = "+";
  }
  else if (spec.sign == ' ')
  {
    sign_str = " ";
  }

  // Convert to string
  unsigned long long abs_val = static_cast<unsigned long long>(value);
  if (abs_val == 0)
  {
    result = "0";
  }
  else
  {
    while (abs_val > 0)
    {
      int digit = static_cast<int>(abs_val % static_cast<unsigned long long>(base));
      if (digit < 10)
      {
        result.push_back(static_cast<char>('0' + digit));
      }
      else
      {
        result.push_back(static_cast<char>((uppercase ? 'A' : 'a') + digit - 10));
      }
      abs_val /= static_cast<unsigned long long>(base);
    }
    std::reverse(result.begin(), result.end());
  }

  // Apply zero padding
  if (spec.zero_pad && spec.width > 0)
  {
    int content_width = static_cast<int>(sign_str.size() + prefix.size() + result.size());
    if (content_width < spec.width)
    {
      result = std::string(static_cast<size_t>(spec.width - content_width), '0') + result;
    }
  }

  result = sign_str + prefix + result;

  // Apply alignment (use right align by default for numbers)
  char align = spec.align;
  if (align == '\0')
  {
    align = '>';
  }
  format_spec align_spec = spec;
  align_spec.align       = align;
  return apply_alignment(result, align_spec);
}

std::string format_float_impl(double value, const format_spec& spec)
{
  std::ostringstream oss;

  // Handle special values
  if (std::isnan(value))
  {
    return apply_alignment("nan", spec);
  }
  if (std::isinf(value))
  {
    return apply_alignment(value < 0 ? "-inf" : "inf", spec);
  }

  // Sign handling
  if (value >= 0 || std::signbit(value) == false)
  {
    if (spec.sign == '+')
    {
      oss << '+';
    }
    else if (spec.sign == ' ')
    {
      oss << ' ';
    }
  }

  // Precision
  int precision = spec.precision >= 0 ? spec.precision : 6;

  // Format type
  switch (spec.type)
  {
  case 'e':
    oss << std::scientific << std::setprecision(precision) << value;
    break;
  case 'E':
    oss << std::scientific << std::uppercase << std::setprecision(precision)
        << value;
    break;
  case 'f':
  case 'F':
    oss << std::fixed << std::setprecision(precision) << value;
    break;
  case 'g':
    oss << std::setprecision(precision) << value;
    break;
  case 'G':
    oss << std::uppercase << std::setprecision(precision) << value;
    break;
  default:
    oss << std::setprecision(precision) << value;
    break;
  }

  std::string result = oss.str();

  // Apply zero padding
  if (spec.zero_pad && spec.width > 0 &&
      static_cast<int>(result.size()) < spec.width)
  {
    size_t insert_pos = 0;
    if (!result.empty() && (result[0] == '-' || result[0] == '+' || result[0] == ' '))
    {
      insert_pos = 1;
    }
    result.insert(insert_pos,
                  static_cast<size_t>(spec.width) - result.size(),
                  '0');
  }

  // Apply alignment
  char align = spec.align;
  if (align == '\0')
  {
    align = '>';
  }
  format_spec align_spec = spec;
  align_spec.align       = align;
  return apply_alignment(result, align_spec);
}

} // namespace

// ============================================================================
// Formatter Implementations
// ============================================================================

// Integer formatter
template<typename T>
void formatter<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>::parse(
    std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

template<typename T>
std::string formatter<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>::format(
    T value) const
{
  return format_integer_impl(static_cast<long long>(value),
                             m_spec,
                             std::is_unsigned_v<T>);
}

// Explicit instantiations for integer types
template struct formatter<short>;
template struct formatter<unsigned short>;
template struct formatter<int>;
template struct formatter<unsigned int>;
template struct formatter<long>;
template struct formatter<unsigned long>;
template struct formatter<long long>;
template struct formatter<unsigned long long>;

// Bool formatter
void formatter<bool>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
  // Check if 'd' type is specified for numeric output
  m_as_string = (m_spec.type != 'd' && m_spec.type != 'b' &&
                 m_spec.type != 'o' && m_spec.type != 'x' && m_spec.type != 'X');
}

std::string formatter<bool>::format(bool value) const
{
  // Check type in format() since m_spec may be set directly without calling parse()
  bool as_number = (m_spec.type == 'd' || m_spec.type == 'b' ||
                    m_spec.type == 'o' || m_spec.type == 'x' || m_spec.type == 'X');
  if (as_number)
  {
    return format_integer_impl(value ? 1 : 0, m_spec, true);
  }
  return apply_alignment(value ? "true" : "false", m_spec);
}

// Float formatter
template<typename T>
void formatter<T, std::enable_if_t<std::is_floating_point_v<T>>>::parse(
    std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

template<typename T>
std::string
formatter<T, std::enable_if_t<std::is_floating_point_v<T>>>::format(
    T value) const
{
  return format_float_impl(static_cast<double>(value), m_spec);
}

// Explicit instantiations for floating-point types
template struct formatter<float>;
template struct formatter<double>;
template struct formatter<long double>;

// const char* formatter
void formatter<const char*>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<const char*>::format(const char* value) const
{
  if (value == nullptr)
  {
    return apply_alignment("(null)", m_spec);
  }
  std::string_view sv(value);
  if (m_spec.precision >= 0 && static_cast<size_t>(m_spec.precision) < sv.size())
  {
    sv = sv.substr(0, static_cast<size_t>(m_spec.precision));
  }
  return apply_alignment(sv, m_spec);
}

// char* formatter
void formatter<char*>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<char*>::format(char* value) const
{
  if (value == nullptr)
  {
    return apply_alignment("(null)", m_spec);
  }
  std::string_view sv(value);
  if (m_spec.precision >= 0 && static_cast<size_t>(m_spec.precision) < sv.size())
  {
    sv = sv.substr(0, static_cast<size_t>(m_spec.precision));
  }
  return apply_alignment(sv, m_spec);
}

// std::string formatter
void formatter<std::string>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<std::string>::format(const std::string& value) const
{
  std::string_view sv = value;
  if (m_spec.precision >= 0 && static_cast<size_t>(m_spec.precision) < sv.size())
  {
    sv = sv.substr(0, static_cast<size_t>(m_spec.precision));
  }
  return apply_alignment(sv, m_spec);
}

// std::string_view formatter
void formatter<std::string_view>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<std::string_view>::format(std::string_view value) const
{
  std::string_view sv = value;
  if (m_spec.precision >= 0 && static_cast<size_t>(m_spec.precision) < sv.size())
  {
    sv = sv.substr(0, static_cast<size_t>(m_spec.precision));
  }
  return apply_alignment(sv, m_spec);
}

// char formatter
void formatter<char>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<char>::format(char value) const
{
  if (m_spec.type == 'd' || m_spec.type == 'x' || m_spec.type == 'X' ||
      m_spec.type == 'o' || m_spec.type == 'b' || m_spec.type == 'B')
  {
    return format_integer_impl(static_cast<int>(value), m_spec, false);
  }
  return apply_alignment(std::string_view(&value, 1), m_spec);
}

// Pointer formatter
template<typename T>
void formatter<T*, std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, char>>>::parse(
    std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

template<typename T>
std::string formatter<T*, std::enable_if_t<!std::is_same_v<std::remove_cv_t<T>, char>>>::format(
    T* value) const
{
  if (value == nullptr)
  {
    return apply_alignment("(nil)", m_spec);
  }

  format_spec ptr_spec = m_spec;
  ptr_spec.alt_form    = true;
  ptr_spec.type        = 'x';

  return format_integer_impl(reinterpret_cast<uintptr_t>(value), ptr_spec, true);
}

// Explicit instantiation for void*
template struct formatter<void*>;
template struct formatter<const void*>;

// nullptr_t formatter
void formatter<std::nullptr_t>::parse(std::string_view spec)
{
  m_spec = parse_format_spec(spec);
}

std::string formatter<std::nullptr_t>::format(std::nullptr_t) const
{
  return apply_alignment("(nil)", m_spec);
}

// ============================================================================
// Format Context Implementation
// ============================================================================

namespace detail
{

std::string format_arg::format(const format_spec& spec) const
{
  if (!m_format_fn || !m_value)
  {
    throw std::invalid_argument("invalid format argument");
  }
  return m_format_fn(m_value, spec);
}

const format_arg& format_args::get(size_t index) const
{
  if (index >= m_size)
  {
    throw std::out_of_range("format argument index out of range");
  }
  return m_args[index];
}

// ============================================================================
// Core vformat Implementation
// ============================================================================

std::string vformat(std::string_view fmt, format_args args)
{
  std::string result;
  result.reserve(fmt.size() * 2);

  size_t auto_index = 0;
  size_t pos        = 0;

  while (pos < fmt.size())
  {
    // Find next '{' or '}'
    size_t open         = fmt.find('{', pos);
    size_t close_escape = fmt.find('}', pos);

    // Handle '}}' that appears before any '{'
    if (close_escape != std::string_view::npos &&
        (open == std::string_view::npos || close_escape < open))
    {
      // Append text before '}'
      result.append(fmt.substr(pos, close_escape - pos));

      // Check for escaped '}}'
      if (close_escape + 1 < fmt.size() && fmt[close_escape + 1] == '}')
      {
        result.push_back('}');
        pos = close_escape + 2;
        continue;
      }
      else
      {
        throw std::invalid_argument("unmatched '}' in format string");
      }
    }

    if (open == std::string_view::npos)
    {
      result.append(fmt.substr(pos));
      break;
    }

    // Append text before '{'
    result.append(fmt.substr(pos, open - pos));

    // Check for escaped '{{'
    if (open + 1 < fmt.size() && fmt[open + 1] == '{')
    {
      result.push_back('{');
      pos = open + 2;
      continue;
    }

    // Find closing '}'
    size_t close = fmt.find('}', open);
    if (close == std::string_view::npos)
    {
      throw std::invalid_argument("unmatched '{' in format string");
    }

    // Parse replacement field
    std::string_view field = fmt.substr(open + 1, close - open - 1);

    // Parse index and format spec
    size_t arg_index = 0;
    std::string_view spec;

    size_t colon = field.find(':');
    if (colon != std::string_view::npos)
    {
      std::string_view index_part = field.substr(0, colon);
      spec                        = field.substr(colon + 1);

      if (index_part.empty())
      {
        arg_index = auto_index++;
      }
      else
      {
        for (char ch : index_part)
        {
          if (ch < '0' || ch > '9')
          {
            throw std::invalid_argument("invalid format field index");
          }
          arg_index = arg_index * 10 + static_cast<size_t>(ch - '0');
        }
      }
    }
    else
    {
      if (field.empty())
      {
        arg_index = auto_index++;
      }
      else
      {
        bool is_number = true;
        for (char ch : field)
        {
          if (ch < '0' || ch > '9')
          {
            is_number = false;
            break;
          }
        }
        if (is_number)
        {
          for (char ch : field)
          {
            arg_index = arg_index * 10 + static_cast<size_t>(ch - '0');
          }
        }
        else
        {
          throw std::invalid_argument("invalid format field");
        }
      }
    }

    // Get argument and format
    const format_arg& arg       = args.get(arg_index);
    format_spec parsed_spec     = parse_format_spec(spec);
    result.append(arg.format(parsed_spec));

    pos = close + 1;
  }

  return result;
}

} // namespace detail

} // namespace fb
