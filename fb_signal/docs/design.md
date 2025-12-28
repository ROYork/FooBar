# fb_signal Design Document

## Overview

`fb_signal` is a high-performance, header-only signal/slot library for C++17 designed for High-Frequency Trading (HFT) applications. It provides type-safe, thread-safe decoupled communication with nanosecond-scale latency.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         signal<Args...>                         │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                    slot_list<Args...>                   │    │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐     │    │
│  │  │ slot_1  │  │ slot_2  │  │ slot_3  │  │  ...    │     │    │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘     │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
        │                                              │
        │ emit() [lock-free]                           │ connect() [mutex]
        ▼                                              ▼
   ┌─────────┐                                    ┌─────────┐
   │ Atomic  │                                    │  Copy-  │
   │Snapshot │                                    │ On-Write│
   └─────────┘                                    └─────────┘
```

## Design Decisions

### 1. Lock-Free Emission via Copy-on-Write

**Choice:** Use `std::shared_ptr<const vector>` with atomic operations.

**Rationale:**
- Emissions take an atomic snapshot (single `atomic_load`)
- Modifications create new vector copy under mutex
- Slots are never modified in-place during iteration
- No locks on hot path

**Trade-off:** Higher memory usage during modifications (copy + original), but HFT prioritizes latency over memory.

### 2. Small-Buffer Optimization (SBO) for Callables

**Choice:** 56-byte inline storage for callable objects.

**Rationale:**
- Most HFT callbacks are small lambdas with few captures
- Lambda with 6 pointer captures ≈ 48 bytes (fits in SBO)
- Avoids heap allocation for typical use cases
- Larger callables fall back to heap (cold path only)

**Storage layout:**
```cpp
struct callable_storage {
  char buffer[56];      // Inline storage
  invoke_fn invoke;     // 8 bytes
  destroy_fn destroy;   // 8 bytes
  move_fn move;         // 8 bytes
  bool is_heap;         // 1 byte (+ padding)
};
```

### 3. Slot Deactivation vs. Removal

**Choice:** Disconnect marks slots as inactive; actual removal is deferred.

**Rationale:**
- O(1) disconnect (atomic flag set)
- Safe for concurrent emission (slot stays in list)
- Cleanup happens during next connect or explicit `cleanup()` call
- No iterator invalidation during emission

### 4. Connection Handle Design

**Choice:** Type-erased `connection` with internal `slot_interface`.

**Rationale:**
- Single `connection` type works with all signal types
- Weak reference semantics (knows if signal is destroyed)
- Copyable for flexible ownership patterns
- `scoped_connection` adds RAII for automatic cleanup

### 5. Priority-Based Ordering

**Choice:** Priority is resolved at connection time via sorted insertion.

**Rationale:**
- No runtime sorting during emission
- Stable order for same-priority slots (connection order preserved)
- Standard priority levels (`low`, `normal`, `high`)

### 6. Event Queue for Cross-Thread Delivery

**Choice:** Lock-free MPSC ring buffer with fixed capacity.

**Rationale:**
- Bounded memory (4096 entries)
- Power-of-two size for efficient modulo operations
- Lock-free enqueue with CAS loop
- Single-consumer simplifies dequeue

**Overflow handling:** Configurable (drop newest or drop oldest).

## Thread Safety Model

| Operation | Lock | Notes |
|-----------|------|-------|
| `emit()` | None | Lock-free atomic snapshot |
| `connect()` | Mutex | Held briefly for copy-on-write |
| `disconnect()` | None | Atomic flag set |
| `block()/unblock()` | None | Atomic flag operations |

**Key guarantee:** Emissions never block on connect/disconnect operations.

## Memory Layout

### Cache-Line Alignment

Critical atomic variables are aligned to 64-byte cache lines to prevent false sharing:

```cpp
alignas(64) std::atomic<std::size_t> m_head{0};  // Consumer
alignas(64) std::atomic<std::size_t> m_tail{0};  // Producers
```

### Slot Entry Layout

```cpp
struct slot_entry {
  callable_storage  m_callable;   // 80 bytes (56 + pointers + flag)
  priority          m_priority;   // 4 bytes
  id_type           m_id;         // 8 bytes
  atomic<bool>      m_active;     // 1 byte
  atomic<bool>      m_blocked;    // 1 byte
  // padding to alignment
};
```

## Performance Characteristics

| Operation | Complexity | Allocation |
|-----------|------------|------------|
| `emit()` (N slots) | O(N) | None |
| `connect()` | O(N) | Yes (new vector) |
| `disconnect()` | O(1) | None |
| `block()/unblock()` | O(1) | None |
| `cleanup()` | O(N) | Yes (new vector) |

## File Structure

```
include/fb/
├── fb_signal.hpp         # Convenience include-all
├── signal.hpp             # Main signal class
├── connection.hpp         # Connection handles
├── slot.hpp               # Callable wrapper
├── event_queue.hpp        # Cross-thread queue
└── detail/
    ├── slot_list.hpp      # Lock-free container
    ├── atomic_utils.hpp   # Atomic helpers
    └── function_traits.hpp # Type traits
```

## Limitations

1. **No return values:** Slots must return `void`
2. **No propagation control:** Cannot stop emission mid-way
3. **Fixed queue size:** Event queue has compile-time capacity
4. **C++17 required:** Uses `std::shared_ptr` atomic operations
