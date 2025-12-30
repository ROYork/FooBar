# fb_core - Core Utilities Library

## Overview

**fb_core** is a foundational C++17 utilities library providing essential components for high-performance applications. It offers platform abstractions and reusable utilities that serve as building blocks for higher-level components.

### Key Features

- **Zero External Dependencies**: Uses only C++17 standard library
- **High Performance**: Designed for low-latency applications
- **Thread-Safe**: All components are thread-safe by default
- **STL Compatible**: Follows STL conventions and patterns
- **Well-Tested**: Comprehensive unit test coverage

## Quick Start

### Timer with Signal/Slot

```cpp
#include <fb/timer.h>

fb::Timer timer;
timer.timeout.connect([]() {
    std::cout << "Tick!" << std::endl;
});
timer.start(1000);  // Fire every second
```

### Circular Buffer

```cpp
#include <fb/circular_buffer.h>

fb::circular_buffer<int> buffer(100);  // Capacity of 100
buffer.push_back(42);
int value = buffer.front();
buffer.pop_front();
```

### CSV Parser

```cpp
#include <fb/csv_parser.h>

fb::csv_parser parser("data.csv");
for (const auto& row : parser) {
    std::cout << row[0] << ", " << row[1] << std::endl;
}
```

### Stopwatch

```cpp
#include <fb/stop_watch.h>

fb::stop_watch sw;
// ... perform operation ...
auto elapsed = sw.elapsed_time();
std::cout << elapsed.count() << " ns" << std::endl;
```

---

## Library Components

| Component | Header | Description |
|-----------|--------|-------------|
| **Timer** | `timer.h` | Event timer with signal/slot integration |
| **Circular Buffer** | `circular_buffer.h` | Fixed-capacity FIFO with STL interface |
| **CSV Parser** | `csv_parser.h` | RFC 4180 compliant CSV parsing |
| **Stop Watch** | `stop_watch.h` | High-resolution timing utilities |
| **Span Compat** | `span_compat.h` | C++17 compatible span type |

---

## API Reference

| Document | Description |
|----------|-------------|
| [timer.md](timer.md) | Timer class with signal integration |
| [circular_buffer.md](circular_buffer.md) | STL-style circular buffer |
| [csv_parser.md](csv_parser.md) | CSV file parsing |
| [stop_watch.md](stop_watch.md) | Elapsed time measurement |

---

## Build Instructions

```bash
cd fb_core
mkdir build && cd build
cmake ..
cmake --build .
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `FB_CORE_BUILD_TESTS` | ON | Build unit tests |
| `FB_CORE_BUILD_EXAMPLES` | ON | Build example programs |

---

## Dependencies

**Runtime**: None (pure C++17 standard library)

**Build**: CMake 3.16+

**Testing**: Google Test

---

*fb_core - Essential utilities for modern C++*
