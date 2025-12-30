# fb_strings Documentation

Comprehensive C++17 string utilities library.

## Documentation Index

| Document | Description |
|----------|-------------|
| **[index.md](index.md)** | Library overview, quick start, installation |
| **[string_utils.md](string_utils.md)** | Core string manipulation (60+ functions) |
| **[string_list.md](string_list.md)** | String container with filtering/sorting |
| **[format.md](format.md)** | String formatting (like std::format) |
| **[string_builder.md](string_builder.md)** | Efficient string concatenation |
| **[utf8_utils.md](utf8_utils.md)** | UTF-8 code point operations |
| **[hashing.md](hashing.md)** | FNV-1a and CRC-32 hashing |
| **[random.md](random.md)** | Random strings & UUID generation |
| **[iterators.md](iterators.md)** | Line/token iterators |

## Quick Start

```cpp
#include <fb/string_utils.h>

std::cout << fb::format("Hello, {}!", "World") << std::endl;
```

See [index.md](index.md) for full documentation.
