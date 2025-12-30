# fb_core Documentation

Core foundational utilities library for C++17.

## Documentation Index

| Document | Description |
|----------|-------------|
| **[index.md](index.md)** | Library overview, quick start, installation |
| **[timer.md](timer.md)** | Event timer with signal/slot integration |
| **[circular_buffer.md](circular_buffer.md)** | STL-style fixed-capacity circular buffer |
| **[csv_parser.md](csv_parser.md)** | RFC 4180 compliant CSV parser |
| **[stop_watch.md](stop_watch.md)** | High-resolution timing utilities |

## Quick Start

```cpp
#include <fb/fb_core.h>

fb::Timer timer;
timer.timeout.connect([]() { std::cout << "Tick!" << std::endl; });
timer.start(1000);  // Fire every second
```

See [index.md](index.md) for full documentation.
