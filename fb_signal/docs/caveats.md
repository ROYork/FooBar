# fb_signal Caveats and Gotchas

## Critical Performance Considerations

### 1. Avoid Heap Allocation in Slots

**Problem:** Heap allocation in slot callbacks adds latency.

```cpp
// BAD - Allocates on every emission
sig.connect([](int x) {
  std::string msg = "Value: " + std::to_string(x);  // Heap allocation!
  log(msg);
});

// GOOD - Pre-allocate or use fixed-size buffer
char buffer[64];
sig.connect([&buffer](int x) {
  snprintf(buffer, sizeof(buffer), "Value: %d", x);
  log(buffer);
});
```

### 2. Capture Size Affects Allocation

Lambdas exceeding 56 bytes trigger heap allocation at connect time.

```cpp
// GOOD - Small capture (fits in SBO)
int a, b, c;
sig.connect([a, b, c](int x) { /* ... */ });  // ~24 bytes, inline

// BAD - Large capture (heap allocated)
std::array<int, 20> large_data;
sig.connect([large_data](int) { /* ... */ });  // ~80 bytes, heap
```

**Rule of thumb:** Keep captures under 6-7 pointers.

### 3. std::function Adds Overhead

`std::function` has its own type-erasure overhead. Direct lambdas are faster.

```cpp
// SLOWER
std::function<void(int)> func = [](int x) { /* ... */ };
sig.connect(func);

// FASTER
sig.connect([](int x) { /* ... */ });
```

## Thread Safety Gotchas

### 4. Object Lifetime with Member Functions

The library does NOT extend object lifetime.

```cpp
// DANGEROUS
{
  Handler handler;
  sig.connect(&handler, &Handler::on_value);
}  // handler destroyed!

sig.emit(42);  // UNDEFINED BEHAVIOR - dangling pointer

// SAFE - Use scoped_connection
class Handler {
  fb::scoped_connection m_conn;
public:
  void subscribe(fb::signal<int>& sig) {
    m_conn = sig.connect([this](int x) { handle(x); });
  }
  // Connection auto-disconnects in destructor
};
```

### 5. Reference Captures and Thread Safety

Reference captures can cause data races.

```cpp
int shared_value = 0;

// DANGEROUS - Data race!
sig.connect([&shared_value](int x) {
  shared_value += x;  // Not atomic!
});

// SAFE - Use atomic
std::atomic<int> safe_value{0};
sig.connect([&safe_value](int x) {
  safe_value.fetch_add(x, std::memory_order_relaxed);
});
```

### 6. Connection Operations from Multiple Threads

While the library is thread-safe, be careful with connection handle access:

```cpp
fb::connection conn;  // Shared

// Thread 1
conn.disconnect();

// Thread 2
if (conn.connected()) {  // May be stale by now!
  conn.block();
}

// SAFER - Each thread has its own handle
auto conn1 = sig.connect([](int) {});  // Thread 1's copy
auto conn2 = conn1;  // Thread 2's copy - same underlying slot
```

## Behavioral Gotchas

### 7. Move-Only Types with Multiple Slots

For move-only types (e.g., `std::unique_ptr`), only the first slot receives valid data.

```cpp
fb::signal<std::unique_ptr<int>> sig;

sig.connect([](std::unique_ptr<int> p) {
  std::cout << "First: " << (p ? *p : -1) << "\n";
});
sig.connect([](std::unique_ptr<int> p) {
  std::cout << "Second: " << (p ? *p : -1) << "\n";  // Gets nullptr!
});

sig.emit(std::make_unique<int>(42));
// Output:
// First: 42
// Second: -1  <- Moved-from value!
```

**Design note:** For copyable types, each slot receives a copy of the arguments.
For move-only or rvalue-reference types, the first slot receives the value via
move, and subsequent slots receive moved-from (empty) values. This is an
inherent limitation of broadcasting move-only types.

**Recommendation:** Use `std::shared_ptr` or pass by const reference for
multi-subscriber scenarios.

### 8. Priority Is Set at Connect Time (cannot be changed)

Changing priority after connection is not supported.

```cpp
auto conn = sig.connect([](int) {}, fb::priority::low);
// There's no: conn.set_priority(fb::priority::high);

// Workaround: Disconnect and reconnect
conn.disconnect();
conn = sig.connect([](int) {}, fb::priority::high);
```

### 8. Slots Added During Emission

New connections made during emission may or may not fire in the current emission.

```cpp
sig.connect([&sig](int) {
  sig.connect([](int x) { std::cout << "New: " << x << "\n"; });
});

sig.emit(1);  // "New: 1" may or may not print
sig.emit(2);  // "New: 2" definitely prints
```

**Recommendation:** Don't rely on this behavior.

### 9. Disconnect During Emission Order

If slot A disconnects slot B, and B hasn't fired yet, B will NOT fire.

```cpp
fb::connection conn_b;

sig.connect([&conn_b](int) {
  std::cout << "A\n";
  conn_b.disconnect();  // Disconnect B before it fires
}, fb::priority::high);

conn_b = sig.connect([](int) {
  std::cout << "B\n";  // May not print!
}, fb::priority::low);

sig.emit(0);  // Prints: A (B was disconnected before reaching it)
```

### 10. Self-Disconnect Doesn't Stop Current Invocation

```cpp
fb::connection conn;

conn = sig.connect([&conn](int) {
  std::cout << "Starting\n";
  conn.disconnect();
  std::cout << "Ending\n";  // STILL PRINTS - function continues
});

sig.emit(0);
// Output:
// Starting
// Ending
```

The slot function runs to completion; disconnect only affects future emissions.

## Memory and Performance

### 11. Cleanup Is Not Automatic

Disconnected slots remain in memory until `cleanup()` or next connection.

```cpp
for (int i = 0; i < 10000; ++i) {
  auto conn = sig.connect([](int) {});
  conn.disconnect();
}
// Memory for 10000 slots still held!

sig.cleanup();  // Now memory is released

// Or connect triggers cleanup
sig.connect([](int) {});  // Also cleans up dead slots
```

### 12. Copy-on-Write Memory Spike

During connection/disconnection, two copies of the slot list exist temporarily.

```cpp
// If you have 1000 slots and connect one more:
// Peak memory = ~2000 slots worth (briefly)

// For memory-sensitive applications, consider:
// - Batching connection changes
// - Calling cleanup() during low-activity periods
```

### 13. Event Queue Overflow

The event queue has fixed capacity (4096 by default). Under heavy load:

```cpp
// Events may be dropped!
for (int i = 0; i < 10000; ++i) {
  queue.enqueue([i]() { /* ... */ });  // Some will fail!
}

// Check success:
if (!queue.enqueue([](){ /* ... */ })) {
  // Handle overflow
}

// Monitor drops:
std::cout << "Dropped: " << queue.dropped_count() << "\n";
```

## API Gotchas

### 14. No Return Values

Slots must return `void`. Aggregating return values is not supported.

```cpp
// NOT POSSIBLE
int result = sig.emit(42);  // Won't compile

// Workaround - Output parameter
int result;
sig.connect([&result](int x) { result = x * 2; });
sig.emit(21);
// result == 42
```

### 15. Connection Default-Constructs as Invalid

```cpp
fb::connection conn;  // Invalid!

if (conn.connected()) {  // false
  // Never reached
}

conn.disconnect();  // Safe but no-op
```

### 16. scoped_connection Must Be Stored

```cpp
// BUG - Immediately disconnected!
sig.connect([](int) {});  // Returns connection, but discarded

// CORRECT
auto conn = sig.connect([](int) {});  // Stored, stays connected

// OR with scoped_connection
fb::scoped_connection sc = sig.connect([](int) {});
```

## Build and Portability

### 17. Requires C++17

The library uses:
- `std::shared_ptr` atomic operations
- `if constexpr`
- Fold expressions
- Structured bindings (internal)

Compiling with C++14 or earlier will fail.

### 18. -fno-exceptions Compatibility

The library is designed to work with exceptions disabled. However:

```cpp
// Heap allocation failure with -fno-exceptions = std::terminate
// If you're memory-constrained, pre-connect slots during initialization
```

### 19. Platform-Specific Pause Instructions

The spin-wait uses platform-specific pause instructions:

```cpp
// x86_64: __builtin_ia32_pause()
// ARM64: asm volatile("yield")
// Others: No-op (may cause higher CPU usage)
```

## Debugging Tips

### 20. Use connection.id() for Debugging

```cpp
auto conn = sig.connect([](int) {});
std::cout << "Connected with ID: " << conn.id() << "\n";
```

### 21. Monitor Slot Count

```cpp
std::cout << "Active slots: " << sig.slot_count() << "\n";
std::cout << "Is empty: " << sig.empty() << "\n";
```

### 22. Event Queue Statistics

```cpp
auto& queue = fb::thread_event_queue();
std::cout << "Pending: " << queue.pending_count() << "\n";
std::cout << "Dropped: " << queue.dropped_count() << "\n";
```
