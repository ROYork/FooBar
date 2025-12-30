# fb::format - String Formatting Library

## Overview

The `fb::format` function provides Python/C++20-style string formatting for C++17. It offers a type-safe, readable alternative to `printf` and string streams.

**Key Features:**

- Positional and sequential argument support
- Type-safe variadic templates
- Format specifiers (width, alignment, precision, type)
- Support for integers, floats, strings, booleans, pointers
- Multiple number bases (decimal, hex, octal, binary)
- Custom alignment and fill characters

## Quick Start

```cpp
#include <fb/format.h>

// Simple formatting
std::string s1 = fb::format("Hello, {}!", "world");
// Result: "Hello, world!"

// Sequential arguments
std::string s2 = fb::format("{} + {} = {}", 2, 3, 5);
// Result: "2 + 3 = 5"

// Format specifiers
std::string s3 = fb::format("Value: {:05d}", 42);
// Result: "Value: 00042"
```

---

## Table of Contents
- [Basic Usage](#basic-usage)
- [Positional Arguments](#positional-arguments)
- [Format Specification](#format-specification)
- [String Formatting](#string-formatting)
- [Integer Formatting](#integer-formatting)
- [Floating-Point Formatting](#floating-point-formatting)
- [Other Types](#other-types)
- [Error Handling](#error-handling)
- [Extending for Custom Types](#extending-for-custom-types)

---

## Basic Usage

```cpp
#include <fb/format.h>

std::string s = fb::format("Hello, {}!", "World");
// s = "Hello, World!"
```

The `{}` placeholder is replaced with the next argument in order.

```cpp
fb::format("{} + {} = {}", 1, 2, 3);  // "1 + 2 = 3"
```

### Escaping Braces

Use `{{` and `}}` to include literal braces:

```cpp
fb::format("Use {{}} for placeholders");
// "Use {} for placeholders"
```

---

## Positional Arguments

Use `{0}`, `{1}`, etc. to specify argument positions:

```cpp
fb::format("{1} before {0}", "second", "first");
// "first before second"

fb::format("{0} {0} {0}", "echo");
// "echo echo echo"
```

---

## Format Specification

Format specifications follow the pattern:

```
{[index][:[[fill]align][sign][#][0][width][.precision][type]]}
```

### Components

| Component | Description | Example |
|-----------|-------------|---------|
| `fill` | Fill character | `*` in `{:*>10}` |
| `align` | Alignment: `<` left, `>` right, `^` center | `{:<10}` |
| `sign` | Sign: `+` always, `-` negative only, ` ` space | `{:+}` |
| `#` | Alternate form (0x for hex, etc.) | `{:#x}` |
| `0` | Zero-pad numbers | `{:05}` |
| `width` | Minimum field width | `{:10}` |
| `.precision` | Decimal precision or max string length | `{:.2f}` |
| `type` | Type specifier | `{:x}` |

---

## String Formatting

### Width and Alignment

```cpp
fb::format("{:10}", "hi");     // "hi        " (left, default)
fb::format("{:<10}", "hi");    // "hi        " (left)
fb::format("{:>10}", "hi");    // "        hi" (right)
fb::format("{:^10}", "hi");    // "    hi    " (center)
```

### Fill Character

```cpp
fb::format("{:*<10}", "hi");   // "hi********"
fb::format("{:*>10}", "hi");   // "********hi"
fb::format("{:*^10}", "hi");   // "****hi****"
```

### Precision (max length)

```cpp
fb::format("{:.3}", "hello");  // "hel"
```

---

## Integer Formatting

### Basic

```cpp
fb::format("{}", 42);     // "42"
fb::format("{}", -42);    // "-42"
```

### Width and Padding

```cpp
fb::format("{:5}", 42);   // "   42" (right-aligned)
fb::format("{:05}", 42);  // "00042" (zero-padded)
```

### Sign

```cpp
fb::format("{:+}", 42);   // "+42"
fb::format("{:+}", -42);  // "-42"
fb::format("{: }", 42);   // " 42" (space for positive)
```

### Bases

```cpp
// Hexadecimal
fb::format("{:x}", 255);    // "ff"
fb::format("{:X}", 255);    // "FF"
fb::format("{:#x}", 255);   // "0xff"
fb::format("{:#X}", 255);   // "0XFF"

// Octal
fb::format("{:o}", 8);      // "10"
fb::format("{:#o}", 8);     // "010"

// Binary
fb::format("{:b}", 10);     // "1010"
fb::format("{:#b}", 10);    // "0b1010"
fb::format("{:#B}", 10);    // "0B1010"
```

---

## Floating-Point Formatting

### Basic

```cpp
fb::format("{}", 3.14159);     // "3.14159"
```

### Precision

```cpp
fb::format("{:.2f}", 3.14159);  // "3.14"
fb::format("{:.4f}", 3.14159);  // "3.1416"
fb::format("{:.0f}", 3.7);      // "4"
```

### Fixed vs Scientific

```cpp
fb::format("{:f}", 1234.5);     // "1234.500000"
fb::format("{:e}", 1234.5);     // "1.234500e+03"
fb::format("{:E}", 1234.5);     // "1.234500E+03"
fb::format("{:g}", 1234.5);     // "1234.5" (general)
```

### Width and Padding

```cpp
fb::format("{:10.2f}", 3.14);    // "      3.14"
fb::format("{:010.2f}", 3.14);   // "0000003.14"
```

### Sign

```cpp
fb::format("{:+.2f}", 3.14);     // "+3.14"
```

### Special Values

```cpp
fb::format("{}", std::numeric_limits<double>::infinity());   // "inf"
fb::format("{}", -std::numeric_limits<double>::infinity());  // "-inf"
fb::format("{}", std::nan(""));                              // "nan"
```

---

## Other Types

### Boolean

```cpp
fb::format("{}", true);     // "true"
fb::format("{}", false);    // "false"
fb::format("{:d}", true);   // "1"
fb::format("{:d}", false);  // "0"
```

### Character

```cpp
fb::format("{}", 'A');      // "A"
fb::format("{:d}", 'A');    // "65"
fb::format("{:x}", 'A');    // "41"
```

### Pointer

```cpp
int x = 42;
fb::format("{}", static_cast<void*>(&x));  // "0x7fff5fbff8cc"
fb::format("{}", nullptr);                 // "(nil)"
```

---

## Error Handling

### Exceptions

`fb::format` throws exceptions on errors:

```cpp
// Unmatched brace
fb::format("{");  // throws std::invalid_argument

// Argument index out of range
fb::format("{10}", 1);  // throws std::out_of_range
```

### Exception-Free Version

Use `fb::try_format` for an exception-free alternative:

```cpp
auto result = fb::try_format("Hello, {}!", "World");
if (result) {
  std::cout << *result << std::endl;
} else {
  std::cout << "Format error" << std::endl;
}
```

---

## Extending for Custom Types

To format custom types, specialize the `fb::formatter` template:

```cpp
struct Point {
  double x, y;
};

template<>
struct fb::formatter<Point> {
  fb::format_spec m_spec;

  void parse(std::string_view spec) {
    m_spec = fb::parse_format_spec(spec);
  }

  std::string format(const Point& p) const {
    return fb::format("({}, {})", p.x, p.y);
  }
};

// Usage
Point p{1.5, 2.5};
fb::format("Point: {}", p);  // "Point: (1.5, 2.5)"
```

---

## Examples

### Table Formatting

```cpp
std::cout << fb::format("{:<10} {:>8} {:>8}\n", "Name", "Score", "Grade");
std::cout << fb::format("{:<10} {:>8} {:>8}\n", "Alice", 95, "A");
std::cout << fb::format("{:<10} {:>8} {:>8}\n", "Bob", 87, "B");
```

Output:
```
Name          Score    Grade
Alice            95        A
Bob              87        B
```

### Hex Dump

```cpp
for (unsigned char c : data) {
  std::cout << fb::format("{:02x} ", c);
}
```

### Currency

```cpp
fb::format("${:,.2f}", 1234567.89);  // Note: grouping not implemented
fb::format("${:.2f}", 1234.56);      // "$1234.56"
```

### Date/Time (example)

```cpp
fb::format("{:04}-{:02}-{:02}", 2025, 1, 15);  // "2025-01-15"
fb::format("{:02}:{:02}:{:02}", 14, 30, 0);    // "14:30:00"
```

---

## Performance Notes

- Format strings are parsed at runtime
- For maximum performance in hot paths, consider pre-building strings
- The implementation avoids heap allocations where possible
- String results are returned by value (benefits from move semantics)

---

## Comparison with std::format (C++20)

| Feature | fb::format | std::format (C++20) |
|---------|------------|---------------------|
| Positional arguments | Yes | Yes |
| Type safety | Yes (runtime) | Yes (compile-time) |
| Format specifiers | Yes (most) | Yes (all) |
| Custom types | Yes | Yes |
| Compile-time validation | No | Yes |
| Locale support | No | Yes |

---

## What's NOT Implemented

- **Compile-time format string checking**: Errors detected at runtime
- **Locale support**: All formatting is locale-independent
- **Dynamic width/precision from args**: Not supported
- **Digit grouping**: Thousands separator not supported

---

## See Also

- [string_builder.md](string_builder.md) - Efficient string concatenation
- [string_utils.md](string_utils.md) - String conversion functions
