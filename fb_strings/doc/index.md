# fb_strings - C++17 String Utilities Library

## Overview

**fb_strings** is a comprehensive C++17 string utilities library providing Qt-inspired string manipulation capabilities for standard C++. The library fills the gap between basic `std::string` functionality and the rich string manipulation features found in frameworks like Qt.

### Key Features

- **Zero External Dependencies**: Uses only C++17 standard library
- **Modern C++17**: Leverages `std::string_view`, `std::optional`, and modern features
- **Comprehensive**: 60+ string manipulation functions covering all common operations
- **Well-Tested**: Extensive unit test coverage
- **Production-Ready**: Built for reliability

### Why Use fb_strings?

**Problem**: Standard C++ strings lack many common operations. Simple tasks like trimming whitespace, splitting strings, or case-insensitive comparison require verbose, error-prone code.

**Solution**: fb_strings provides a complete toolkit with a clean, consistent API:

```cpp
// Without fb_strings: verbose, error-prone
std::string s = "  hello world  ";
s.erase(0, s.find_first_not_of(" \t"));
s.erase(s.find_last_not_of(" \t") + 1);
std::transform(s.begin(), s.end(), s.begin(), ::toupper);

// With fb_strings: clean, readable
std::string s = fb::to_upper(fb::trim("  hello world  "));
```

---

## Quick Start

### Installation

Include fb_strings in your project via CMake:

```cmake
add_subdirectory(fb_strings)
target_link_libraries(myapp fb_strings)
```

Or include the headers directly:

```cpp
#include <fb/string_utils.h>  // Core utilities
#include <fb/format.h>        // Formatting
#include <fb/string_list.h>   // String list container
```

### Basic Usage

```cpp
#include <iostream>
#include <fb/string_utils.h>
#include <fb/format.h>
#include <fb/string_list.h>

using namespace fb;

int main() {
    // 1. Format strings (like std::format/Python)
    std::string msg = format("Hello, {}! Score: {:.2f}", "Alice", 95.5);
    std::cout << msg << std::endl;  // "Hello, Alice! Score: 95.50"

    // 2. String list operations
    string_list files = {"doc.txt", "image.png", "data.csv"};
    auto txt_files = files.filter_ends_with(".txt");
    std::cout << txt_files.join(", ") << std::endl;  // "doc.txt"

    // 3. String manipulation
    std::string input = "  Hello World  ";
    std::cout << trim(input) << std::endl;          // "Hello World"
    std::cout << to_lower(input) << std::endl;      // "  hello world  "

    // 4. Splitting and joining
    auto parts = split("a,b,c", ',');
    std::cout << join(parts, " - ") << std::endl;   // "a - b - c"

    return 0;
}
```

---

## Library Components

| Component | Header | Description |
|-----------|--------|-------------|
| **String Utils** | `string_utils.h` | 60+ core string manipulation functions |
| **String List** | `string_list.h` | Container for string collections with filtering/sorting |
| **Format** | `format.h` | Type-safe string formatting (like std::format) |
| **String Builder** | `string_builder.h` | Efficient string concatenation |
| **UTF-8 Utils** | `utf8_utils.h` | UTF-8 code point operations |
| **Hashing** | `string_hash.h` | FNV-1a and CRC-32 hashing |
| **Random** | `string_random.h` | Random string generation, UUID |
| **Iterators** | `string_iterators.h` | Line/token iterators for range-based for |

---

## API Reference

| Document | Description |
|----------|-------------|
| [string_utils.md](string_utils.md) | Core string manipulation functions |
| [string_list.md](string_list.md) | String container class |
| [format.md](format.md) | String formatting |
| [string_builder.md](string_builder.md) | String builder class |
| [utf8_utils.md](utf8_utils.md) | UTF-8 operations |
| [hashing.md](hashing.md) | String hashing |
| [random.md](random.md) | Random string generation |
| [iterators.md](iterators.md) | Line/token iterators |

---

## Comparison with std Library  

fb_strings provides functionality not available in C++17:

| Feature | fb_strings | C++17 std | C++20 std |
|---------|------------|-----------|-----------|
| `starts_with()` / `ends_with()` | Yes | No | Yes |
| String formatting | Yes `fb::format` | No | Yes `std::format` |
| String splitting | Yes `split()` | No | No |
| Case conversion | Yes `to_upper()` | No | No |
| Natural sort | Yes `natural_compare()` | No | No |
| String list container | Yes `string_list` | No | No |
| Levenshtein distance | Yes | No | No |
| URL/HTML encoding | Yes | No | No |

---

## Build Instructions

```bash
cd fb_strings
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Tests

```bash
ctest --output-on-failure
```

---

## Platform Support

### Compilers
- **GCC**: 7.0+
- **Clang**: 5.0+
- **MSVC**: Visual Studio 2017+
- **Apple Clang**: Xcode 10+

### C++17 Features Used
- `std::string_view` for efficient string passing
- `std::optional` for optional return values
- `if constexpr` for compile-time branches
- Structured bindings
- Fold expressions

---

## Dependencies

**Runtime**: None (pure C++17 standard library)

**Build**: CMake 3.16+

**Testing**: Google Test

---

*fb_strings - Powerful string utilities for modern C++*
