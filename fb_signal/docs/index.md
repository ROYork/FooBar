# fb_signal - High-Performance Signal/Slot Library

## Overview

**fb_signal** is a high-performance signal/slot library designed for low-latency applications including High Frequency Trading (HFT). It provides type-safe, decoupled communication between components with minimal overhead.

### Key Features

- **Type-Safe**: Compile-time type checking for signal arguments
- **High Performance**: Optimized for low-latency applications
- **Thread-Safe**: Safe to use across multiple threads
- **Automatic Lifetime Management**: Scoped connections and connection guards
- **Priority Ordering**: Control slot execution order
- **Filtered Connections**: Only invoke slots when conditions are met
- **RAII Connection Management**: Automatic disconnect on scope exit

## Quick Start

```cpp
#include <fb/signal.hpp>

// Create a signal with int argument
fb::signal<int> on_value_changed;

// Connect a lambda
auto conn = on_value_changed.connect([](int value) {
    std::cout << "Value: " << value << std::endl;
});

// Emit the signal
on_value_changed.emit(42);  // Prints: Value: 42

// Disconnect when done
conn.disconnect();
```

---

## Library Components

| Component | Header | Description |
|-----------|--------|-------------|
| **Signal** | `signal.hpp` | Core signal class for emitting events |
| **Slot** | `slot.hpp` | Callable wrapper for receivers |
| **Connection** | `connection.hpp` | Connection lifecycle management |
| **Event Queue** | `event_queue.hpp` | Cross-thread event dispatching |

---

## Core Concepts

### Signal

A signal is an observable event source. When emitted, all connected slots are invoked.

```cpp
fb::signal<int, std::string> data_received;
data_received.emit(42, "hello");
```

### Connection

A connection represents the link between a signal and a slot. It can be connected, disconnected, or blocked.

```cpp
auto conn = signal.connect(handler);
conn.block();     // Temporarily disable
conn.unblock();   // Re-enable
conn.disconnect(); // Permanently remove
```

### Scoped Connection

RAII wrapper that automatically disconnects when destroyed.

```cpp
{
    fb::scoped_connection sc = signal.connect(handler);
    // Connection active
}  // Automatically disconnected
```

---

## API Reference

| Document | Description |
|----------|-------------|
| [usage.md](usage.md) | Comprehensive usage examples |
| [design.md](design.md) | Design decisions and architecture |
| [caveats.md](caveats.md) | Gotchas and best practices |

---

## Comparison with Other Libraries

| Feature | fb_signal | Boost.Signals2 | Qt Signals |
|---------|-----------|----------------|------------|
| Header-only | Yes | No | No |
| Thread-safe | Yes | Yes | Yes |
| Type-safe | Yes | Yes | Limited |
| Zero dependencies | Yes | No | No |
| Priority ordering | Yes | Yes | No |
| Filtered connections | Yes | No | No |
| Connection blocking | Yes | Yes | No |

---

## What's NOT Implemented

- **Queued cross-thread emission**: Use event_queue for async dispatch
- **Return value collection**: Signals are void-returning
- **Automatic thread affinity**: Manual thread management required

---

## Build Instructions

```bash
cmake -S . -B ./build/
cmake --build build -j8
ctest --test-dir build/ --output-on-failure
```

---

*fb_signal - Fast, type-safe signal/slot for modern C++*
