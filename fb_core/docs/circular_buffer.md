# Circular Buffer - Fixed-Capacity FIFO Container

## Overview

`fb::circular_buffer<T>` is an STL-style fixed-capacity circular buffer (ring buffer). It provides O(1) push and pop operations at both ends, making it ideal for producer-consumer queues, sliding windows, and bounded caches.

**Key Features:**

- STL-compatible interface (iterators, begin/end, etc.)
- Fixed capacity determined at construction
- O(1) push_back, pop_front operations
- Automatic overwriting when full (optional)
- Random access to elements
- Thread-safe when externally synchronized

## Quick Start

```cpp
#include <fb/circular_buffer.h>

fb::circular_buffer<int> buffer(5);  // Capacity of 5

buffer.push_back(1);
buffer.push_back(2);
buffer.push_back(3);

std::cout << buffer.front();  // 1
buffer.pop_front();

for (int val : buffer) {
    std::cout << val << " ";  // 2 3
}
```

---

## Constructors

```cpp
circular_buffer(size_t capacity);                    // Create with capacity
circular_buffer(InputIt first, InputIt last);        // From iterator range
circular_buffer(const circular_buffer& other);       // Copy
```

---

## Capacity

| Method | Description |
|--------|-------------|
| `size()` | Current number of elements |
| `capacity()` | Maximum capacity |
| `empty()` | True if no elements |
| `full()` | True if size == capacity |

---

## Element Access

| Method | Description |
|--------|-------------|
| `front()` | Access first element |
| `back()` | Access last element |
| `operator[n]` | Access by index (unchecked) |
| `at(n)` | Access by index (bounds-checked) |

```cpp
buffer[0];           // First element (unchecked)
buffer.at(0);        // First element (throws if out of range)
buffer.front();      // Same as buffer[0]
buffer.back();       // Last element
```

---

## Modifiers

| Method | Description |
|--------|-------------|
| `push_back(val)` | Add to end (overwrites oldest if full) |
| `pop_front()` | Remove from front |
| `clear()` | Remove all elements |

```cpp
buffer.push_back(42);  // Add to end
buffer.pop_front();    // Remove from front
buffer.clear();        // Remove all
```

---

## Iterators

Full STL-compatible iterator support:

```cpp
for (auto it = buffer.begin(); it != buffer.end(); ++it) {
    std::cout << *it << std::endl;
}

// Range-based for
for (const auto& item : buffer) {
    std::cout << item << std::endl;
}

// Reverse iteration
for (auto it = buffer.rbegin(); it != buffer.rend(); ++it) {
    std::cout << *it << std::endl;
}
```

---

## Example: Sliding Window Average

```cpp
fb::circular_buffer<double> window(10);  // Last 10 values

void add_sample(double value) {
    window.push_back(value);  // Automatically drops oldest when full
}

double average() {
    double sum = 0;
    for (double val : window) sum += val;
    return window.empty() ? 0 : sum / window.size();
}
```

---

## Comparison with std

| Feature | circular_buffer | std::vector | std::deque |
|---------|-----------------|-------------|------------|
| Fixed capacity | Yes | No | No |
| O(1) push_back | Yes | Amortized | Yes |
| O(1) pop_front | Yes | No (O(n)) | Yes |
| Overwrite when full | Yes | N/A | N/A |
| Random access | Yes | Yes | Yes |

---

## What's NOT Implemented

- **Dynamic resizing**: Capacity is fixed at construction
- **push_front**: Only push_back is supported
- **pop_back**: Only pop_front is supported
- **emplace operations**: Use push_back with constructed values

---

## See Also

- [timer.md](timer.md) - Event timer
- [index.md](index.md) - Library overview
