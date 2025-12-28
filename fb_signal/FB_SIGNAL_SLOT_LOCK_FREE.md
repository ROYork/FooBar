# Signal/Slot Library Requirements — Open Design

## 1. Overview

Design and implement a high-performance signal/slot library for use in High Frequency Trading (HFT) applications. The library should provide a type-safe, thread-safe mechanism for decoupled component communication.

**Intent:** This document describes the **spirit, constraints, and performance requirements** of the library. The API design, naming conventions, and implementation details are left to the designer's discretion. Focus on achieving the stated goals rather than matching a prescribed interface.

---

## 2. Design Philosophy

The library should embody these principles:

| Principle | Description |
|-----------|-------------|
| **Performance First** | Nanosecond-scale latency on the hot path; every cycle counts |
| **Predictability** | Deterministic behavior; no hidden allocations or locks during emission |
| **Type Safety** | Compile-time guarantees; slot signatures must be verifiable |
| **Modern C++** | Leverage C++17/20 idioms; no macro magic or preprocessor tricks |
| **Minimal Footprint** | Header-only, zero external dependencies |

**Designer Freedom:** You may choose your own:
- Class and function names
- Connection/handle semantics
- Template patterns and metaprogramming techniques
- Internal data structures and algorithms

---

## 3. Hard Constraints

These are non-negotiable requirements:

| Constraint | Specification |
|------------|---------------|
| Language | C++17 required; C++20 features optional |
| Dependencies | Standard library only (no Boost, Qt, TBB, etc.) |
| Distribution | Header-only library |
| Exceptions | Must compile with `-fno-exceptions` |
| Namespace | `fb` (for toolbox) |

---

## 4. Performance Requirements

### 4.1 Latency Targets

These represent **maximum acceptable latencies** under typical conditions:

| Operation | Target |
|-----------|--------|
| Signal emission with 1 slot (direct) | < 100 ns |
| Signal emission with 10 slots (direct) | < 500 ns |
| Establishing a connection | < 1 µs |
| Removing a connection | < 500 ns |
| Queuing a cross-thread event | < 100 ns |

**Measurement Context:** x86_64 Linux, modern CPU (3+ GHz), warm cache

### 4.2 Memory Rules

**Hot Path (Emission):**
- **Zero heap allocation** — `malloc`, `new`, allocators are forbidden
- This is non-negotiable; no fallback allocation is acceptable

**Cold Path (Connect/Disconnect):**
- Heap allocation is permitted
- Memory usage should scale linearly with connection count

### 4.3 Concurrency Guarantees

**Emission (Hot Path):**
- Must be thread-safe (concurrent emissions from multiple threads)
- Must be **lock-free** (no mutexes, spinlocks, or unbounded retry loops)
- One slot executing must not block other emissions

**Connection Management (Cold Path):**
- Must be thread-safe
- May use locks, but must never block active emissions

---

## 5. Functional Requirements

The following capabilities must be provided. The **mechanism** for providing them is the designer's choice.

### 5.1 Signal/Slot Core

A signal must be able to:
- Accept variadic template arguments (arbitrary types)
- Have multiple slots connected to it
- Invoke all connected slots when triggered
- Support querying connection count and empty state

### 5.2 Callable Support

The library must support connecting these callable types:
- Lambda expressions (capturing and non-capturing)
- Free functions
- Member function pointers (both const and non-const)
- Functor objects (classes with `operator()`)
- `std::function` wrappers

### 5.3 Connection Lifecycle

Connections should provide:
- **RAII semantics** — automatic cleanup when handle goes out of scope
- **Explicit control** — ability to manually disconnect
- **Blocking** — temporarily disable a connection without disconnecting
- **State queries** — check if connected, blocked, etc.

Consider providing scoped/guarded connection helpers for managing multiple connections.

### 5.4 Delivery Policies

Support at least these invocation modes:

| Mode | Behavior |
|------|----------|
| Direct | Invoke immediately in the emitting thread |
| Queued | Defer invocation to the receiver's thread |
| Automatic | Direct if same thread, queued otherwise |

Provide a mechanism for the receiver thread to process queued invocations.

### 5.5 Cross-Thread Communication

For queued connections:
- Provide a per-thread event processing mechanism
- Queue must be thread-safe for multiple producers (MPSC pattern recommended)
- Events must be delivered in FIFO order
- Lock-free enqueue is required

**Recommended implementation:** Pre-sized ring buffer (e.g., 4096 entries, power-of-two)

### 5.6 Optional Advanced Features

These features are desirable but implementation approach is at designer's discretion:

| Feature | Description |
|---------|-------------|
| **Priority** | Higher-priority slots invoked before lower; equal priority maintains connection order |
| **Filtering** | Conditional slot invocation based on argument predicate |
| **Bulk disconnect** | Remove all connections from a signal |

---

## 6. Reentrancy & Edge Cases

The library must handle these scenarios gracefully:

| Scenario | Required Behavior |
|----------|-------------------|
| Reentrant emission | Slot may emit same or different signal during invocation |
| Disconnect during emission | Disconnected slot must not be called if not yet reached |
| Self-disconnection | Slot may safely disconnect itself |
| New connection during emission | May or may not fire in current emission (implementation-defined) |
| Infinite recursion | Not prevented (caller's responsibility) |

---

## 7. Platform & Toolchain

### 7.1 Target Environments

| Priority | Platform |
|----------|----------|
| Primary | Linux/x86_64 (production HFT) |
| Secondary | macOS/ARM64 (development) |

**Assumptions:**
- Cache line: 64 bytes
- Strong memory ordering (x86 TSO), but use explicit orderings for portability
- No Windows support required initially

### 7.2 Compiler Support

- **Required:** GCC 9+, Clang 10+ with `-std=c++17`
- **Optional C++20:** Concepts, `std::atomic::wait()`, `std::source_location`

---

## 8. Testing Requirements

### 8.1 Coverage Areas

The implementation must include comprehensive tests for:

1. **Basic Operations** — emission, single/multiple slots, various argument types
2. **Connection Management** — RAII, explicit disconnect, blocking, move semantics
3. **Callable Types** — lambdas, free functions, member functions, functors
4. **Advanced Features** — priority ordering, filtering, bulk disconnect
5. **Thread Safety** — concurrent emission, concurrent connect/disconnect, queued delivery
6. **Edge Cases** — reentrancy, self-disconnection, modification during emission

### 8.2 Framework & Validation

- Use **Google Test** (GTest)
- All tests must pass with **ThreadSanitizer** (TSan) enabled
- Tests should validate latency claims where practical

---

## 9. Deliverables

1. **Source Code**
   - Header-only library in `include/fb_signals/`
   - All implementation in header files

2. **Unit Tests**
   - Test files in `ut/tb_signals/`
   - CMake integration for test discovery

3. **Build System**
   - CMakeLists.txt for library and tests

---

## 10. Evaluation Criteria

Implementations will be assessed on:

| Criterion | Weight | Focus |
|-----------|--------|-------|
| **Correctness** | High | All functional requirements met, tests pass |
| **Performance** | High | Meets or exceeds latency targets |
| **Thread Safety** | High | No races, passes TSan |
| **Design Quality** | Medium | Elegant API, appropriate abstractions |
| **Code Quality** | Medium | Clean, readable, maintainable |

---

## 11. Designer Guidance

### What This Document Prescribes

✓ Performance budgets and constraints  
✓ Required capabilities and behaviors  
✓ Thread safety guarantees  
✓ Testing and validation requirements

### What This Document Does NOT Prescribe

✗ Class names or method signatures  
✗ Specific template patterns  
✗ Internal data structures  
✗ Connection handle implementation details  
✗ Error handling mechanisms beyond exception-free operation

### Design Questions to Consider

As you design the API, consider:

1. How will users discover the API? (IDE autocomplete, documentation)
2. What compile-time errors will users see for type mismatches?
3. How will you balance ergonomics vs. performance?
4. What tradeoffs exist between features (e.g., priority adds overhead)?
5. How will queued connections handle non-trivially-copyable types?

---

## 12. Reference Notes

- Refer to `Coding_Standards.md` for coding conventions
- Refer to `Project_Info.md` for project structure guidelines
- Do not reference third-party signal/slot libraries in code or comments

---

## Document History

| Version | Date | Description |
|---------|------|-------------|
| 1.0 | 2025-12-14 | Initial open-design requirements |
