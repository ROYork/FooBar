# String Builder - Efficient String Concatenation

## Overview

`fb::string_builder` is an efficient string concatenation class with buffer reuse. It provides a fluent API for building strings incrementally.

**Key Features:**

- Pre-allocation to avoid repeated reallocations
- Method chaining for fluent API
- Append operations for strings, characters, and numeric types
- Insert, remove, and replace operations
- Formatted string appending
- Stream-style operators for convenience

## Quick Start

```cpp
#include <fb/string_builder.h>

fb::string_builder sb;
sb << "Hello" << ' ' << 42 << '\n';
sb.append("World");
sb.append_format("{:.2f}", 3.14);
std::string result = sb.to_string();
```

---

## Constructors

```cpp
string_builder();                           // Empty builder
string_builder(size_t initial_capacity);    // Pre-allocate capacity
string_builder(std::string_view initial);   // Initialize with content
```

---

## Append Operations

| Method | Description |
|--------|-------------|
| `append(str)` | Append string view |
| `append(char)` | Append character |
| `append(char, count)` | Append n copies of char |
| `append_line(str="")` | Append with newline |
| `append_format(fmt, args...)` | Append formatted string |
| `append_int(val)` | Append integer (32/64-bit) |
| `append_uint(val)` | Append unsigned integer |
| `append_double(val, prec)` | Append double with precision |
| `append_bool(val)` | Append "true" or "false" |

```cpp
fb::string_builder sb;
sb.append("Hello");
sb.append(' ');
sb.append('.', 3);
sb.append_line("World");
sb.append_format("{}: {}", "Value", 42);
// "Hello ...World\nValue: 42"
```

---

## Modification Operations

| Method | Description |
|--------|-------------|
| `insert(pos, str)` | Insert at position |
| `remove(pos, count)` | Remove characters |
| `replace(from, to)` | Replace first occurrence |
| `replace_all(from, to)` | Replace all occurrences |
| `clear()` | Clear content |

---

## Access Operations

| Method | Description |
|--------|-------------|
| `to_string()` | Get result as string (copy) |
| `view()` | Get string_view (no copy) |
| `size()` | Current length |
| `capacity()` | Buffer capacity |
| `empty()` | Check if empty |
| `reserve(cap)` | Reserve capacity |

---

## Stream Operators

```cpp
fb::string_builder sb;
sb << "Name: " << name;
sb << 42;
sb << 3.14;
sb << true;  // "true"
```

Supported types: `const char*`, `std::string_view`, `char`, `int32_t`, `int64_t`, `uint32_t`, `uint64_t`, `double`, `bool`.

---

## Example

```cpp
fb::string_builder sb;
sb.append_line("Report");
sb.append_line("======");
sb.append_format("Date: {}\n", "2024-01-15");
sb.append_format("Items: {}\n", 42);
sb.append_format("Total: ${:.2f}\n", 199.99);

std::cout << sb.to_string();
```

---

## Comparison with std

| Feature | string_builder | std::string | std::ostringstream |
|---------|---------------|-------------|-------------------|
| Buffer reuse | Yes | Yes | No |
| Fluent API | Yes | No | No |
| Format support | Yes | No | No |
| Performance | High | High | Lower |

---

## What's NOT Implemented

- **Thread safety**: Not thread-safe, use separate instances per thread
- **Copy semantics**: Move-only for efficiency

---

## See Also

- [format.md](format.md) - String formatting
- [string_utils.md](string_utils.md) - String utilities
