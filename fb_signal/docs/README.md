# fb_signal Documentation

High-performance signal/slot library for C++17.

## Documentation Index

| Document | Description |
|----------|-------------|
| **[index.md](index.md)** | Library overview, quick start |
| **[usage.md](usage.md)** | Comprehensive usage examples |
| **[design.md](design.md)** | Design decisions and rationale |
| **[caveats.md](caveats.md)** | Gotchas and best practices |

## Quick Start

```cpp
#include <fb/signal.hpp>

fb::signal<int> value_changed;

value_changed.connect([](int val) {
    std::cout << "Value: " << val << std::endl;
});

value_changed.emit(42);  // Prints: Value: 42
```

See [index.md](index.md) for full documentation.
