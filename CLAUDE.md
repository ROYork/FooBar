# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure and build (Linux/macOS)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Run all tests
./build/fb_signal/ut/fb_signals_tests
./build/fb_core/ut/fb_core_unit_tests

# Run specific test by filter
./build/fb_signal/ut/fb_signals_tests --gtest_filter="SignalTest.*"
./build/fb_core/ut/fb_core_unit_tests --gtest_filter="TimerTest.*"

# Build with ThreadSanitizer
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_TSAN=ON
cmake --build build -j
```

## Project Overview

FooBar is a collection of high-performance C++ utility libraries designed for HFT (High-Frequency Trading) applications. It consists of three libraries:

| Library | Type | Purpose |
|---------|------|---------|
| **fb_signal** | Header-only | Lock-free signal/slot system with nanosecond-scale emission |
| **fb_strings** | Static library | String manipulation and formatting utilities |
| **fb_core** | Mixed | Core utilities (timer, circular_buffer, stop_watch) |

All libraries use the `fb` namespace and target C++17.

## Architecture

### fb_signal - Lock-Free Signal/Slot

Key design patterns:
- **Copy-on-Write**: Emissions take atomic snapshot of slot list (lock-free). Modifications create new vector under mutex.
- **Small-Buffer Optimization**: 56-byte inline storage for callables avoids heap allocation on hot path.
- **Deferred Cleanup**: `disconnect()` sets atomic flag (O(1)); actual removal happens during next `connect()` or explicit `cleanup()`.

Thread safety model:
- `emit()` - Lock-free (atomic snapshot)
- `connect()` - Brief mutex hold
- `disconnect()`, `block()`, `unblock()` - Atomic flag operations only

Performance targets: <100ns emission with 1 slot, <500ns with 10 slots.

### fb_strings - String Utilities

Design decisions:
- ASCII-only for case operations (predictable performance)
- Returns `std::optional<T>` for parsing failures (no exceptions)
- Accepts `std::string_view` for inputs, returns `std::string` for outputs
- `string_list` class supports method chaining: `.trim_all().remove_empty().sort()`

### fb_core - Core Utilities

- `timer` - High-precision timer with fb_signal integration for timeout events
- `stop_watch` - Simple elapsed time measurement
- `circular_buffer` - Fixed-size lock-free ring buffer
- `span_compat` - C++17 polyfill for std::span

Dependencies: fb_core depends on fb_signal for timer's timeout signal.

## Coding Conventions

- **Naming**: snake_case for classes, methods, and variables (e.g., `timer`, `is_active()`, `set_interval()`)
- **Headers**: `.hpp` for fb_signal, `.h` for fb_strings and fb_core
- **Namespace**: Everything in `fb` namespace
- **Error handling**: `std::optional` for expected failures; exceptions only for programmer errors
- **Thread safety**: Mutexes protecting const methods must be declared `mutable`

## Key Constraints

- Compiles with `-fno-exceptions` (heap failures → `std::terminate`)
- Zero heap allocation on emission hot path
- 64-byte cache-line alignment for atomics to prevent false sharing
- Designed for x86_64 TSO memory model; explicit memory ordering on ARM
