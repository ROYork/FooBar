# Stop Watch - High-Resolution Timing

## Overview

`fb::stop_watch` is a high-resolution stopwatch for measuring elapsed time. It uses `std::chrono::steady_clock` to provide nanosecond resolution timing with all operations being thread-safe.

**Key Features:**

- Nanosecond resolution using steady_clock
- Thread-safe operations
- Start, stop, resume functionality
- Accumulated time across multiple runs
- Auto-start option on construction

## Quick Start

```cpp
#include <fb/stop_watch.h>

fb::stop_watch sw;  // Starts automatically

// ... perform operation ...

auto elapsed = sw.elapsed_time();
std::cout << elapsed.count() << " nanoseconds" << std::endl;

// Convert to milliseconds
auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
std::cout << ms.count() << " ms" << std::endl;
```

---

## Constructor

```cpp
stop_watch(bool start_immediately = true);
```

By default, the stopwatch starts immediately on construction.

```cpp
fb::stop_watch sw;           // Starts immediately
fb::stop_watch sw(true);     // Starts immediately
fb::stop_watch sw(false);    // Created stopped, call start() to begin
```

---

## Methods

| Method | Description |
|--------|-------------|
| `start()` | Start or resume timing |
| `stop()` | Stop timing (accumulates elapsed time) |
| `reset(start)` | Reset to zero, optionally restart |
| `elapsed_time()` | Get total elapsed time |
| `is_running()` | Check if currently timing |

---

## Usage Examples

### Basic Timing

```cpp
fb::stop_watch sw;
expensive_operation();
auto elapsed = sw.elapsed_time();
```

### Manual Start/Stop

```cpp
fb::stop_watch sw(false);  // Don't start yet

sw.start();
phase1();
sw.stop();

sw.start();  // Resume (accumulates)
phase2();
sw.stop();

// Total time for both phases
auto total = sw.elapsed_time();
```

### Benchmarking Loop

```cpp
fb::stop_watch sw(false);
const int iterations = 1000;

sw.start();
for (int i = 0; i < iterations; ++i) {
    operation_to_benchmark();
}
sw.stop();

auto avg = sw.elapsed_time() / iterations;
std::cout << "Average: " << avg.count() << " ns per op" << std::endl;
```

---

## Thread Safety

All methods are thread-safe:

```cpp
fb::stop_watch sw;

// Thread 1
sw.start();

// Thread 2
auto elapsed = sw.elapsed_time();  // Safe

// Thread 3
sw.stop();  // Safe
```

---

## Comparison with std::chrono

| Feature | stop_watch | Manual chrono |
|---------|------------|---------------|
| Thread-safe | Yes | No |
| Pause/resume | Yes | Manual |
| Accumulation | Yes | Manual |
| RAII-friendly | Yes | Moderate |

---

## What's NOT Implemented

- **Lap timing**: Use multiple stopwatches or manual tracking
- **Statistical analysis**: Collect samples externally
- **Wall-clock time**: Uses steady_clock (monotonic)

---

## See Also

- [timer.md](timer.md) - Event timer with callbacks
- [index.md](index.md) - Library overview
