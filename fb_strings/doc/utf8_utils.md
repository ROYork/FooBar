# UTF-8 Utils - Unicode Code Point Operations

## Overview

The `utf8_utils.h` module provides functions for working with UTF-8 encoded strings at the code point level rather than byte level.

**Key Features:**

- Code point counting and indexing
- UTF-8 aware substring operations
- Validation and sanitization
- Byte/code point position conversion

## Quick Start

```cpp
#include <fb/utf8_utils.h>

std::string text = "Hello, 世界";  // "Hello, World" in Chinese

// Get length in code points (not bytes)
size_t len = fb::utf8_length(text);  // 9 (not 13 bytes)

// Get substring by code point index
std::string left = fb::utf8_left(text, 5);   // "Hello"
std::string right = fb::utf8_right(text, 2); // "世界"

// Validate UTF-8
if (!fb::is_valid_utf8(text)) {
    text = fb::sanitize_utf8(text);
}
```

---

## Length and Indexing

### `utf8_length(str)`

Get length in code points (not bytes).

```cpp
fb::utf8_length("Hello");    // 5
fb::utf8_length("世界");     // 2 (not 6 bytes)
```

### `utf8_at(str, index)`

Get code point at index.

```cpp
fb::utf8_at("Hello, 世界", 7);  // "世"
```

---

## Substring Operations

| Function | Description |
|----------|-------------|
| `utf8_left(str, n)` | Get first n code points |
| `utf8_right(str, n)` | Get last n code points |
| `utf8_mid(str, pos, count)` | Get substring by code point indices |

```cpp
std::string text = "Hello, 世界";
fb::utf8_left(text, 5);      // "Hello"
fb::utf8_right(text, 2);     // "世界"
fb::utf8_mid(text, 7, 2);    // "世界"
```

---

## Validation

### `is_valid_utf8(str)`

Check if string contains valid UTF-8 encoding.

```cpp
fb::is_valid_utf8("Hello");        // true
fb::is_valid_utf8("\xff\xfe");     // false (invalid bytes)
```

### `sanitize_utf8(str, replacement)`

Replace invalid UTF-8 bytes with a replacement character.

```cpp
fb::sanitize_utf8("\xff\xfeHello", '?');  // "??Hello"
```

---

## Position Conversion

| Function | Description |
|----------|-------------|
| `utf8_byte_offset(str, cp_index)` | Convert code point index to byte offset |
| `utf8_codepoint_index(str, byte_offset)` | Convert byte offset to code point index |

```cpp
std::string text = "Hello, 世界";
fb::utf8_byte_offset(text, 7);      // 7 (byte position of "世")
fb::utf8_codepoint_index(text, 10); // 8 (code point index at byte 10)
```

---

## Comparison with std

| Feature | fb_strings | C++17 std | C++20 std |
|---------|------------|-----------|-----------|
| UTF-8 length | Yes | No | No |
| UTF-8 substr | Yes | No | No |
| UTF-8 validation | Yes | No | No |
| Code point indexing | Yes | No | No |

---

## What's NOT Implemented

- **Unicode normalization**: Requires ICU or similar library
- **Case folding for non-ASCII**: Only ASCII case operations
- **Grapheme cluster support**: Code point level only

---

## See Also

- [string_utils.md](string_utils.md) - Core string utilities
- [encoding.md](encoding.md) - URL/HTML encoding
