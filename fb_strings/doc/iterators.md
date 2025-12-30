# Iterators - Zero-Copy String Parsing

## Overview

The `string_iterators.h` module provides iterator classes for parsing strings without creating intermediate string copies. All iterators work with `std::string_view` for zero-copy operation.

**Key Features:**

- Line iterator (handles \r, \n, \r\n)
- Word iterator (whitespace-separated)
- Token iterator (custom delimiters)
- Zero-copy via string_view

## Quick Start

```cpp
#include <fb/string_iterators.h>

std::string text = "line1\nline2\nline3";

// Iterate over lines
fb::line_iterator lines(text);
while (lines.has_next()) {
    std::string_view line = lines.next();
    std::cout << line << std::endl;
}
```

---

## line_iterator

Zero-copy iterator over lines. Handles multiple line ending styles: `\r`, `\n`, and `\r\n`.

```cpp
fb::line_iterator it(text);

while (it.has_next()) {
    std::string_view line = it.next();
    // Process line
}

it.reset();  // Start over
```

---

## word_iterator

Zero-copy iterator over whitespace-separated words. Consecutive whitespace is treated as a single separator.

```cpp
std::string text = "  hello   world  ";
fb::word_iterator it(text);

while (it.has_next()) {
    std::string_view word = it.next();
    // "hello", "world"
}
```

---

## token_iterator

Zero-copy iterator with custom delimiters. Empty tokens between consecutive delimiters are returned.

```cpp
std::string text = "a,b,,c";
fb::token_iterator it(text, ",");

while (it.has_next()) {
    std::string_view token = it.next();
    // "a", "b", "", "c"
}
```

---

## API Reference

### Common Methods

| Method | Description |
|--------|-------------|
| `has_next()` | Check if more items available |
| `next()` | Get next item as string_view |
| `reset()` | Reset to beginning |

### Constructors

```cpp
line_iterator(std::string_view str);
word_iterator(std::string_view str);
token_iterator(std::string_view str, std::string_view delimiters);
```

---

## Example: CSV Parsing

```cpp
std::string csv = "name,age,city\nAlice,30,NYC\nBob,25,LA";

fb::line_iterator lines(csv);
bool first = true;
while (lines.has_next()) {
    std::string_view line = lines.next();
    if (first) { first = false; continue; }  // Skip header
    
    fb::token_iterator fields(line, ",");
    while (fields.has_next()) {
        std::string_view field = fields.next();
        // Process field
    }
}
```

---

## Performance

Iterators provide zero-copy parsing:
- No memory allocation during iteration
- Source string must remain valid during iteration
- Returns `string_view` (O(1) copy)

---

## Comparison with std

| Feature | fb_strings | C++ std |
|---------|------------|---------|
| Line iterator | Yes | No |
| Word iterator | Yes | No |
| Token iterator | Yes | No |
| Zero-copy | Yes | N/A |

---

## What's NOT Implemented

- **Range-based for support**: Use `has_next()`/`next()` pattern
- **Bidirectional iteration**: Forward-only
- **Regex-based splitting**: Use `std::regex` for complex patterns

---

## See Also

- [string_utils.md](string_utils.md) - split() functions
- [string_list.md](string_list.md) - String container
