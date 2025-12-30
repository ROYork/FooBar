# Coding Standards

## Document Scope & Precedence

- This document defines **style and engineering standards** (formatting, naming, documentation, testing, and general practices).
- `Project_Info.md` defines **project/tooling constraints** (C++ standard, CMake, build workflow, folder exclusions).
- If any statement conflicts across docs, resolve in this order: `*_REQUIREMENTS.md` (behavior/API) → `*_Info.md` or `*_Description.md` (tooling/constraints) → `Coding_Standards.md` (style/practices).

## Table of Contents
- [Target Platforms](#target-platforms)
- [C++ Standard](#c-standard)
- [General Guidance](#general-guidance)
- [Project Structure Standards](#project-structure-standards)
- [Naming Conventions](#naming-conventions)
- [Code Formatting and Style](#code-formatting-and-style)
- [Comments and Documentation](#comments-and-documentation)
- [Error Handling](#error-handling)
- [Memory Management](#memory-management)
- [Performance Best Practices](#performance-best-practices)
- [Testing Standards](#testing-standards)
- [Security Considerations](#security-considerations)
- [Version Control Guidelines](#version-control-guidelines)
- [Code Review Process](#code-review-process)
- [Build System](#build-system)
- [Markdown Documentation](#markdown-documentation)

---

## Target Platforms

- Windows (MSVC)
- Linux (G++)
- macOS (Clang)

**Rationale**: Cross-platform compatibility ensures the library can be deployed in diverse HFT environments without modification.

---

## C++ Standard

Code must target **C++17** as the baseline standard. This is **strictly required** for compilation compatibility.

**Optional Feature Detection**: If newer compiler features (C++20/23) can provide performance benefits, they may be used conditionally via compile-time macros:

```cpp
#if __cplusplus >= 202002L
  // Use C++20 features
  std::span<int> data;
#else
  // Fallback to C++17
  int* data;
  size_t size;
#endif
```

**Rationale**: C++17 provides modern language features (structured bindings, `std::optional`, `std::variant`) while maintaining broad compiler support. Conditional newer features enable optimization without breaking compatibility.

---

## General Guidance

### Performance Requirements

This library targets **High-Frequency Trading (HFT)** applications. All code decisions must prioritize:

1. **Speed** - Minimize latency at all costs
2. **Efficiency** - Optimize CPU and memory usage
3. **Low Latency** - Predictable and deterministic performance
4. **Speed over Size** - Binary size is less critical than runtime performance

### Design Principles

- **Unit Testability**: Design all classes and functions to be easily testable in isolation from the start
- **Minimal Macros**: Avoid preprocessor macros; prefer `constexpr`, templates, and inline functions
- **Judicious Inheritance**: Use inheritance only when it provides clear benefits; favor composition over inheritance to avoid unnecessary vtable lookups and improve code clarity
- **Include Guards**: Use `#pragma once` instead of traditional header guards for cleaner, faster compilation

**Rationale**: HFT systems require microsecond-level performance. Virtual function calls, heap allocations, and unpredictable branches can introduce unacceptable latency.

---

## Project Structure Standards

All C++ projects must follow industry-standard directory organization patterns. The structure depends on whether the library is header-only or requires compiled implementation files.

### Traditional Libraries (with Implementation)

For libraries with separate `.h` and `.cpp` files (like Model_7), use this structure:

```
project_name/
├── include/              ← Public API headers ONLY
│   └── namespace/
│       ├── module1.h
│       ├── module2.h
│       └── module3.h
├── src/                  ← Implementation files ONLY
│   ├── module1.cpp
│   ├── module2.cpp
│   └── module3.cpp
├── ut/                   ← Unit tests
│   ├── test_module1.cpp
│   ├── test_module2.cpp
│   └── test_module3.cpp
├── docs/                 ← Documentation
│   ├── index.md
│   ├── module1.md
│   └── examples.md
├── cmake/                ← CMake helper modules (optional)
│   └── FindDependency.cmake
└── CMakeLists.txt        ← Build configuration
```

**Key Principles:**

1. **Separate Public API from Implementation**
   - `include/` contains ONLY public headers users will include
   - `src/` contains ONLY implementation `.cpp` files
   - Never mix `.h` and `.cpp` files in the same directory

2. **Namespace Directory**
   - Headers go in `include/namespace/` (e.g., `include/tb/string_utils.h`)
   - This allows users to write: `#include <tb/string_utils.h>`

3. **CMake Configuration**
   ```cmake
   target_include_directories(target_name
     PUBLIC
       $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
       $<INSTALL_INTERFACE:include>
   )
   
   # Install headers
   install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
   ```

4. **Include Paths in Source Files**
   - In `.cpp` files: `#include <namespace/header.h>`
   - Never use relative paths like `#include "header.h"`

**Example (tb_strings library):**
```
tb_strings/
├── include/tb/
│   ├── string_utils.h
│   ├── format.h
│   └── string_list.h
├── src/
│   ├── string_utils.cpp
│   ├── format.cpp
│   └── string_list.cpp
└── ut/
    ├── test_string_utils.cpp
    └── test_format.cpp
```

**User includes:**
```cpp
#include <tb/string_utils.h>
#include <tb/format.h>
```

### Header-Only Libraries

For header-only libraries (templates, small utilities), use this simpler structure:

```
project_name/
├── include/              ← All headers (public API)
│   └── namespace/
│       ├── module1.h
│       ├── module2.h
│       ├── impl/         ← Implementation details (optional)
│       │   └── internal.h
│       └── module3.h
├── ut/                   ← Unit tests
│   ├── test_module1.cpp
│   └── test_module2.cpp
├── docs/                 ← Documentation
│   └── index.md
└── CMakeLists.txt        ← Build configuration (INTERFACE library)
```

**Key Differences:**

1. **No `src/` Directory**
   - Everything is in headers since there's no compiled code

2. **Optional `impl/` Subdirectory**
   - For implementation details users shouldn't directly include
   - Example: `include/tb/impl/string_helpers.h`

3. **INTERFACE Library**
   ```cmake
   add_library(target_name INTERFACE)
   target_include_directories(target_name INTERFACE
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
     $<INSTALL_INTERFACE:include>
   )
   ```

**Example (header-only utility):**
```
string_view_utils/
├── include/tb/
│   ├── string_view_ops.h
│   └── impl/
│       └── char_traits.h
└── ut/
    └── test_string_view_ops.cpp
```

### Anti-Patterns to Avoid

❌ **Mixing Headers and Implementation:**
```
// BAD - Non-standard structure
project/
└── src/namespace/
    ├── module1.h      ← Headers mixed with implementation
    ├── module1.cpp
    ├── module2.h
    └── module2.cpp
```

❌ **No Namespace Directory:**
```
// BAD - Headers not in namespace subdirectory
project/
├── include/
│   ├── string_utils.h  ← Missing namespace folder
│   └── format.h
└── src/
```

User has to write: `#include <string_utils.h>` (namespace collision risk)

❌ **Implementation in `include/`:**
```
// BAD - .cpp files in include directory
project/
└── include/
    ├── module.h
    └── module.cpp     ← Wrong location
```

### Installation and Distribution

Following these standards ensures:

1. **Standard CMake Integration**
   ```cmake
   find_package(project_name REQUIRED)
   target_link_libraries(my_app PRIVATE project_name::project_name)
   ```

2. **Clean System Installation**
   ```bash
   /usr/local/include/namespace/
   /usr/local/lib/libproject_name.a
   ```

3. **IDE Support**
   - Auto-completion works correctly
   - "Go to definition" navigates properly
   - Include path suggestions are accurate

4. **Package Manager Compatibility**
   - Works with vcpkg, Conan, etc.
   - Standard structure expected by distribution tools

### Rationale

This structure is used by major C++ projects:
- **Boost**: All libraries follow include/src separation
- **LLVM/Clang**: Strict public API (include/) vs implementation (lib/) separation
- **Google libraries**: protobuf, gtest, etc. use this pattern
- **Modern CMake best practices**: Supports proper `BUILD_INTERFACE` and `INSTALL_INTERFACE`

**Benefits:**
- Crystal-clear API boundary
- Prevents accidental dependencies on internal headers
- Simplifies installation and packaging
- Improves discoverability and documentation
- Reduces coupling between library internals and public API

---

### Fixed-Width Integer Types (int8_t, uint32_t, etc.) 

- Use fixed-width types when the exact size matters:
  - Binary formats and serialization: Reading/writing file formats, network protocols, or any external data representation where byte layout is specified
  - Hardware interfaces: Memory-mapped I/O, register definitions, or direct hardware interaction
  - Cryptographic operations: Hash functions, encryption algorithms where bit-width affects correctness
  - Cross-platform data structures: Shared memory, IPC, or any data structure that must have identical layout across platforms
  - Public API contracts: When a function’s signature explicitly promises a fixed range/width or uses sentinel values that should behave identically across platforms (document the rationale)
  - Bit manipulation: When you need specific bit counts for shifts, masks, or packed data structures
  - Fixed-point arithmetic: When emulating specific precision requirements

- Use standard types (int, size_t, ptrdiff_t) when:

  - General application logic: Counters, indices, loop variables where the actual size doesn't matter
  - API boundaries with standard library: Use size_t for sizes/indices, ptrdiff_t for pointer arithmetic
  - Performance-critical code: Let the compiler choose the natural word size (int is typically fastest)
  - Interfacing with C/C++ APIs: Match the types used by the APIs you're calling
  - Container sizes and indices: Use size_t or the container's size_type

**Key Principles:**

Don't use fixed-width types "for clarity" - int is clearer than `int32_t` when size doesn't matter
When in doubt, use `int` for general integers, `size_t` for sizes
Document why when you use fixed-width types - the reason should be clear
Avoid mixing - if a calculation involves both `int` and `uint32_t`, consider whether that's intentional


## Naming Conventions

Clear, consistent naming improves code readability and maintainability.

### Variables and Functions

Use **snake_case** for all variable and function names:

```cpp
// Good
int account_balance = 0;
double calculate_average_price(const std::vector<double>& prices);
std::string user_name;

// Bad
int AccountBalance = 0;
double CalculateAveragePrice(const std::vector<double>& prices);
std::string userName;
```

### Constants

Use **ALL_CAPS** with underscores for compile-time constants:

```cpp
// Good
constexpr uint32_t MAX_RETRIES = 5;
constexpr uint32_t DEFAULT_TIMEOUT_SECONDS = 30;
constexpr double PI = 3.14159265359;

// Bad - macros instead of constexpr
#define maxRetries 5
#define defaultTimeoutSeconds 30

// Bad - wrong casing
constexpr uint32_t MaxRetries = 5;
const uint32_t max_retries = 5;
```

**Rationale**: `constexpr` provides type safety and debuggability that macros lack. ALL_CAPS clearly distinguishes constants from variables.

### Class and Type Names

Use **PascalCase** for class, struct, enum, and type alias names **by default**.

**Exception (utility libraries):** In *utility-style libraries* (generic helper code such as string/containers/algorithms), it is acceptable for **public** types to use **snake_case** when all of the following are true:

- The type lives in a *utility* namespace/module (e.g., `tb` or `tb::str_utils`), not an application/domain namespace.
- The type represents a generic tool/container/adaptor rather than a business/domain concept.
- The library’s public API is predominantly snake_case already (functions, free helpers), and the snake_case type name improves consistency/readability.
- The choice is documented once for the library/module (e.g., in the project architecture or README) and applied consistently within that library.

**Rule of thumb (choose based on what the type “is”):**
- **Domain / end-user / top-level components**: PascalCase (e.g., `TradingEngine`, `OrderBook`, `RiskManager`)
- **Utility library building blocks**: snake_case *allowed* (e.g., `string_list`, `string_builder`, `path_utils`)

```cpp
// Good (domain / end-user types)
class OrderBook;
struct MarketData;
using PriceMap = std::unordered_map<std::string, double>;

// Good (utility library types - allowed when the exception applies)
namespace tb
{
  class string_list;
  class string_builder;
}

// Bad (mixing conventions without a documented reason)
class order_book;
struct market_data;
```

### Class Member Variables

Prefix all member variables with **`m_`**:

```cpp
class TradingEngine
{
public:
  TradingEngine(int max_orders);
  int get_order_count() const { return m_order_count; }

private:
  int m_order_count = 0;
  double m_total_volume = 0.0;
  std::string m_exchange_name;
};
```

**Rationale**: The `m_` prefix instantly distinguishes member variables from local variables and parameters, reducing confusion and preventing naming conflicts.

### Template Parameters

Use **PascalCase** with descriptive names:

```cpp
// Good
template<typename ValueType, typename AllocatorType = std::allocator<ValueType>>
class CircularBuffer;

// Acceptable for simple cases
template<typename T>
class Vector;

// Bad - too cryptic
template<typename VT, typename AT>
class CircularBuffer;
```

### Namespaces

Use **snake_case** for namespace names:

```cpp
namespace string_utils
{
  std::string trim(const std::string& str);
}

namespace hft::order_management
{
  class Order;
}
```

---

## Code Formatting and Style

### Indentation

- Use **2 spaces** for indentation
- **Never use tabs** - configure your editor to convert tabs to spaces
- Maintain consistent indentation depth throughout

```cpp
// Good
class Foo
{
public:
  void bar()
  {
    if (condition)
    {
      do_something();
    }
  }
};

// Bad - inconsistent indentation
class Foo
{
  public:
    void bar()
    {
        if (condition)
        {
            do_something();
        }
    }
};
```

**Rationale**: Consistent 2-space indentation provides clear visual structure without excessive horizontal space consumption.

### Brace Style

Use **Allman style** (braces on their own line):

```cpp
// Good - Allman style
void process_order(Order& order)
{
  if (order.is_valid())
  {
    execute_trade(order);
  }
  else
  {
    reject_order(order);
  }
}

// Bad - K&R style
void process_order(Order& order) {
  if (order.is_valid()) {
    execute_trade(order);
  } else {
    reject_order(order);
  }
}
```

### Always Use Braces

**Always** use braces even for single-statement blocks:

```cpp
// Good
if (m_ptr != nullptr)
{
  return m_ptr->value();
}

// Bad - missing braces
if (m_ptr != nullptr)
  return m_ptr->value();
```

**Rationale**: Braces prevent subtle bugs when code is modified later and improve consistency.

### Line Length

Keep lines under **100 characters** where practical. Break long lines logically.

**Note:** Doxygen comments have a stricter **80-character limit** (see [Comments and Documentation](#comments-and-documentation)).

```cpp
// Good
double result = calculate_complex_metric( parameter_one,
                                          parameter_two,
                                          parameter_three );

// Acceptable for short overruns
std::string message = "This is a reasonably long string that slightly exceeds 100 chars";
```

### Automated Formatting

Use the provided `.clang-format` file to automatically format code. Run before committing:

```bash
clang-format -i src/**/*.cpp src/**/*.h
```

---

## Comments and Documentation

### General Principles

- Write **self-documenting code** with clear names; comments should explain *why*, not *what*
- Avoid redundant comments that merely restate the code
- Keep comments up-to-date; outdated comments are worse than no comments

### Avoid Useless Comments

```cpp
// Bad - states the obvious
/// @brief Default constructor
Socket();

/// @brief Destructor
~Socket();

/// @brief Gets the value
int get_value() const { return m_value; }

// Good - no comment needed when obvious
Socket();
~Socket();
int get_value() const { return m_value; }
```

### Doxygen Documentation

Use Doxygen-style comments for public APIs, complex functions, and non-obvious implementations.

#### Single-Line Comments

For brief descriptions, use triple-slash inline style:

```cpp
/// @brief Creates an uninitialized socket
Socket();

/// @brief Calculates the exponential moving average
double calculate_ema(const std::vector<double>& prices, double alpha);
```

#### Multi-Line Comments

Use C-style block comments for detailed descriptions:

```cpp
/**
 * @brief Socket is the common base class for StreamSocket, ServerSocket,
 * DatagramSocket and other socket classes.
 * 
 * It provides operations common to all socket types including binding,
 * connection management, and data transmission. Socket handles are
 * RAII-managed and automatically closed on destruction.
 * 
 * @note This class is not thread-safe. External synchronization required
 * for concurrent access.
 */
class Socket
{
  // ...
};
```

#### Parameter Documentation

Document parameters, return values, and exceptions:

```cpp
/**
 * @brief Submits an order to the exchange
 * 
 * @param order The order to submit (must be validated)
 * @param timeout_ms Maximum time to wait for confirmation
 * @return Order ID assigned by the exchange, or -1 on failure
 * @throws NetworkException if connection to exchange is lost
 * @throws ValidationException if order fails validation
 * 
 * @note This function blocks until order is acknowledged or timeout occurs
 */
int64_t submit_order(const Order& order, uint32_t timeout_ms);
```

### Inline Comments for Structs

For simple structures, use inline Doxygen comments after member declarations:

```cpp
/**
 * @brief Pulse Data Word (PDW) captures key characteristics of a single
 * radar pulse emitted by the target.
 */
struct PDW
{
  double toa;        ///< Time of Arrival (in nanoseconds since epoch)
  double freq;       ///< Frequency (in Hz)
  double pw;         ///< Pulse Width (in nanoseconds)
  double amp;        ///< Amplitude (in dBm)
  int64_t timestamp; ///< System timestamp for data correlation
};
```

### Implementation Comments

Explain complex algorithms or non-obvious logic:

```cpp
// Calculate Black-Scholes option price using iterative approximation
// This avoids the expensive erf() function call in the standard formula
double price = underlying_price;
for (int i = 0; i < MAX_ITERATIONS; ++i)
{
  // Newton-Raphson step
  double delta = calculate_delta(price, volatility);
  price -= (theoretical_value(price) - market_price) / delta;
}
```

### TODO and FIXME Comments

Mark incomplete work clearly:

```cpp
// TODO(username): Implement connection pooling for better performance
// FIXME(username): Race condition in multi-threaded access to m_cache
// HACK(username): Temporary workaround until exchange API is fixed
```

### Column Limit

Doxygen comments must respect the **80-character column limit** for readability.

### No Emojis

**Do not use emojis** in source code, comments, documentation, or commit messages.

```cpp
// Bad
/// @brief Calculates the order price 💰
/// @warning This function is slow! 🐢

// Good
/// @brief Calculates the order price
/// @warning This function has O(n^2) complexity - use with caution
```

```
# Bad commit messages
feat: Add new trading feature 🚀
fix: Fix memory leak 🐛

# Good commit messages
feat: Add limit order support to matching engine
fix: Resolve memory leak in order book destructor
```

**Rationale**: Emojis introduce encoding issues across different systems, editors, and terminals. They render inconsistently, can break tooling (grep, diff, log parsers), and add no technical value. Professional documentation should rely on clear, descriptive language rather than pictograms.

---

## Error Handling

### Return Values vs Exceptions

**Prefer Return Values for Expected Failures** (hot path):

```cpp
// Good - std::optional for expected failures
std::optional<Order> find_order(int64_t order_id) const
{
  auto it = m_orders.find(order_id);
  if (it != m_orders.end())
  {
    return it->second;
  }
  return std::nullopt;
}

// Usage
if (auto order = engine.find_order(12345))
{
  process_order(*order);
}
```

**Use Exceptions for Unexpected/Unrecoverable Errors**:

```cpp
// Good - exception for exceptional conditions
void load_config(const std::string& filename)
{
  std::ifstream file(filename);
  if (!file)
  {
    throw std::runtime_error("Failed to open config file: " + filename);
  }
  // ...
}
```

**Rationale**: In HFT, exceptions on the hot path can cause unacceptable latency. Use exceptions for initialization, configuration errors, and truly exceptional conditions.

### Error Return Conventions

- Return `std::optional<T>` for operations that may legitimately fail
- Return `bool` only when no additional error information is needed
- Avoid error codes when `std::optional` or `std::variant` is clearer

### Assertion Usage

Use assertions to catch programming errors during development:

```cpp
#include <cassert>

void set_price(double price)
{
  assert(price >= 0.0 && "Price cannot be negative");
  m_price = price;
}
```

For performance-critical paths, provide both safe and unchecked accessors. The safe version always validates, while the unchecked version is for verified hot paths:

```cpp
template<typename T>
class CircularBuffer
{
public:
  // Safe accessor - always validates bounds (use by default)
  T& at(size_t index)
  {
    if (index >= m_size)
    {
      throw std::out_of_range("Index out of bounds");
    }
    return m_data[index];
  }

  // Unchecked accessor - for verified hot paths only
  // Precondition: index < size() must be guaranteed by caller
  T& operator[](size_t index) noexcept
  {
    assert(index < m_size && "Index out of bounds - use at() for safe access");
    return m_data[index];
  }

private:
  T* m_data;
  size_t m_size;
};

// Usage in hot path - caller ensures bounds are valid
void process_batch(CircularBuffer<Order>& buffer, size_t start, size_t count)
{
  // Validate once at boundary
  if (start + count > buffer.size())
  {
    throw std::out_of_range("Batch exceeds buffer bounds");
  }
  
  // Hot loop uses unchecked access (bounds already verified)
  for (size_t i = 0; i < count; ++i)
  {
    buffer[start + i].process(); // Safe: bounds checked above
  }
}
```

**Rationale**: This pattern maintains safety by default while allowing performance optimization where bounds have been pre-validated. The `assert` in `operator[]` catches bugs during development without adding Release overhead.

### Resource Cleanup

Always use RAII for resource management:

```cpp
// Good - RAII ensures cleanup
class FileHandle
{
public:
  explicit FileHandle(const std::string& path)
    : m_file(std::fopen(path.c_str(), "r"))
  {
    if (!m_file)
    {
      throw std::runtime_error("Failed to open file");
    }
  }

  ~FileHandle() { if (m_file) std::fclose(m_file); }
  
  // Prevent copying
  FileHandle(const FileHandle&) = delete;
  FileHandle& operator=(const FileHandle&) = delete;

private:
  FILE* m_file;
};

// Bad - manual cleanup prone to leaks
FILE* file = std::fopen("data.txt", "r");
if (!file) return -1;
// ... code that might return early ...
std::fclose(file); // Might never be reached!
```

---

## Memory Management

### Prefer Stack Allocation

Stack allocation is faster and safer than heap allocation:

```cpp
// Good - stack allocated
void process_data()
{
  std::array<double, 100> prices;
  fill_prices(prices);
}

// Bad - unnecessary heap allocation
void process_data()
{
  auto prices = new double[100];
  fill_prices(prices);
  delete[] prices;
}
```

### Smart Pointers

When heap allocation is necessary, use smart pointers:

```cpp
// Good - unique ownership
std::unique_ptr<OrderBook> m_order_book = std::make_unique<OrderBook>();

// Good - shared ownership (use sparingly)
std::shared_ptr<MarketData> m_market_data = std::make_shared<MarketData>();

// Bad - manual memory management
OrderBook* m_order_book = new OrderBook();
// ... easy to forget delete
```

### Pointer Nullability

Check pointers before dereferencing:

```cpp
void process_order(Order* order)
{
  if (order == nullptr)
  {
    return; // or throw exception
  }
  
  // Safe to use order now
  order->execute();
}
```

### Avoid `new` and `delete`

Direct `new`/`delete` should be rare. Prefer:
- `std::make_unique` / `std::make_shared`
- `std::vector` and other containers
- Stack allocation

```cpp
// Good
auto buffer = std::make_unique<char[]>(size);
std::vector<Order> orders;

// Bad
char* buffer = new char[size];
Order* orders = new Order[100];
```

---

## Performance Best Practices

### Minimize Allocations

Avoid allocations in hot paths:

```cpp
// Good - reuse buffer
class MessageParser
{
public:
  void parse_stream(Stream& stream)
  {
    m_buffer.clear();
    stream.read_into(m_buffer);
    process(m_buffer);
  }

private:
  std::vector<char> m_buffer; // Reused across calls
};

// Bad - allocates every call
void parse_stream(Stream& stream)
{
  std::vector<char> buffer; // New allocation each time
  stream.read_into(buffer);
  process(buffer);
}
```

### Pass by Reference

Avoid expensive copies by passing large objects by reference:

```cpp
// Good
double calculate_average(const std::vector<double>& prices);
void update_order(Order& order);

// Bad - expensive copy
double calculate_average(std::vector<double> prices);
void update_order(Order order);
```

### Use `constexpr` for Compile-Time Computation

```cpp
// Good - computed at compile time
constexpr double PI = 3.14159265359;
constexpr uint32_t BUFFER_SIZE = 1024 * 64;

constexpr uint32_t calculate_size(uint32_t n)
{
  return n * sizeof(double);
}

// Bad - computed at runtime
const double PI = 3.14159265359;
const uint32_t BUFFER_SIZE = 1024 * 64;
```

### Reserve Container Capacity

Pre-allocate when size is known:

```cpp
// Good
std::vector<Order> orders;
orders.reserve(expected_count);
for (int i = 0; i < expected_count; ++i)
{
  orders.push_back(create_order(i));
}

// Bad - multiple reallocations
std::vector<Order> orders;
for (int i = 0; i < expected_count; ++i)
{
  orders.push_back(create_order(i)); // May reallocate
}
```

### Avoid Virtual Functions in Hot Paths

Virtual function calls prevent inlining and add indirection:

```cpp
// Good for performance-critical code
template<typename Strategy>
class TradingEngine
{
  void execute()
  {
    Strategy::calculate(); // Inlined at compile time
  }
};

// Bad for hot paths - vtable lookup overhead
class Strategy
{
  virtual void calculate() = 0;
};
```

### Profile Before Optimizing

Use profiling tools to identify actual bottlenecks:

```cpp
// Add profiling markers for performance analysis
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
critical_operation();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
```

---

## Testing Standards

### Unit Test Requirements

- **All public APIs** must have unit tests
- Tests must be **fast** (< 1ms per test ideally)
- Tests must be **deterministic** (no flaky tests)

### Code Coverage Requirements

Coverage targets vary by code criticality:

| Category | Minimum Coverage | Examples |
|----------|------------------|----------|
| **Critical paths** | 90% | Order execution, matching engine, position tracking |
| **Risk management** | 90% | Risk checks, limit enforcement, margin calculations |
| **Data integrity** | 90% | Serialization, message parsing, state persistence |
| **General business logic** | 80% | Reporting, analytics, non-critical utilities |
| **Infrastructure/utilities** | 70% | Logging, configuration, diagnostic tools |

**Rationale**: In HFT systems, bugs in critical paths can result in significant financial loss. The cost of additional test coverage is far outweighed by the risk of production defects in money-handling code.

### Test Organization

```cpp
// test_order_book.cpp
#include "order_book.h"
#include <gtest/gtest.h>

class OrderBookTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_order_book = std::make_unique<OrderBook>("AAPL");
  }

  std::unique_ptr<OrderBook> m_order_book;
};

TEST_F(OrderBookTest, AddOrder_ValidOrder_OrderAdded)
{
  // Arrange
  Order order(100, 150.50, Side::BUY);

  // Act
  bool result = m_order_book->add_order(order);

  // Assert
  EXPECT_TRUE(result);
  EXPECT_EQ(1, m_order_book->order_count());
}

TEST_F(OrderBookTest, AddOrder_InvalidPrice_ThrowsException)
{
  // Arrange
  Order order(100, -10.0, Side::BUY);

  // Act & Assert
  EXPECT_THROW(m_order_book->add_order(order), std::invalid_argument);
}
```

### Test Naming Convention

Use descriptive names: `MethodName_Scenario_ExpectedBehavior`

```cpp
TEST(CalculatorTest, Add_TwoPositiveNumbers_ReturnsSum)
TEST(CalculatorTest, Divide_ByZero_ThrowsException)
TEST(OrderBook, GetBestBid_EmptyBook_ReturnsNullopt)
```

### Mock Objects

Use mocking frameworks for dependencies:

```cpp
class MockExchange : public IExchange
{
public:
  MOCK_METHOD(bool, submit_order, (const Order&), (override));
  MOCK_METHOD(bool, cancel_order, (int64_t), (override));
};

TEST(TradingEngine, SubmitOrder_ValidOrder_CallsExchange)
{
  MockExchange mock_exchange;
  TradingEngine engine(&mock_exchange);
  Order order(100, 50.0, Side::BUY);

  EXPECT_CALL(mock_exchange, submit_order(order))
    .Times(1)
    .WillOnce(::testing::Return(true));

  engine.submit_order(order);
}
```

### Performance Tests

Include benchmark tests for critical operations:

```cpp
#include <benchmark/benchmark.h>

static void BM_OrderBookInsert(benchmark::State& state)
{
  OrderBook book("AAPL");
  for (auto _ : state)
  {
    Order order(state.range(0), 150.0, Side::BUY);
    book.add_order(order);
  }
}
BENCHMARK(BM_OrderBookInsert)->Range(8, 8<<10);
```

---

## Security Considerations

### Input Validation

**Always validate external input**:

```cpp
// Good - validates input
bool set_order_quantity(int32_t quantity)
{
  if (quantity <= 0 || quantity > MAX_ORDER_QUANTITY)
  {
    return false;
  }
  m_quantity = quantity;
  return true;
}

// Bad - trusts input
void set_order_quantity(int32_t quantity)
{
  m_quantity = quantity; // What if negative or overflow?
}
```

### Integer Overflow

Protect against overflow in calculations:

```cpp
#include <limits>

// Good - checks for overflow
bool safe_multiply(uint32_t a, uint32_t b, uint32_t& result)
{
  if (a == 0 || b == 0)
  {
    result = 0;
    return true;
  }

  if (a > std::numeric_limits<uint32_t>::max() / b)
  {
    return false; // Overflow would occur
  }

  result = a * b;
  return true;
}
```

### Avoid Buffer Overflows

```cpp
// Good - bounds checking
void copy_data(const char* src, size_t src_len, char* dest, size_t dest_len)
{
  size_t copy_len = std::min(src_len, dest_len - 1);
  std::memcpy(dest, src, copy_len);
  dest[copy_len] = '\0';
}

// Bad - potential overflow
void copy_data(const char* src, char* dest)
{
  strcpy(dest, src); // Unsafe!
}
```

### Secrets Management

Never hardcode credentials or secrets:

```cpp
// Bad
const std::string API_KEY = "sk_live_abcd1234";

// Good - load from secure configuration
std::string load_api_key()
{
  return std::getenv("API_KEY");
}
```

### Secure Random Numbers

Use cryptographically secure random generation when needed:

```cpp
#include <random>

// Good - for security-sensitive operations
std::random_device rd;
std::mt19937_64 gen(rd());
std::uniform_int_distribution<> dis(0, 100);

// Note: For HFT, prefer faster PRNGs for non-security uses
```

---

## Version Control Guidelines

### Commit Messages

Use clear, descriptive commit messages:

```
Format: <type>: <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code refactoring
- `perf`: Performance improvement
- `test`: Adding or updating tests
- `docs`: Documentation changes
- `build`: Build system changes

**Good Examples**:
```
feat: Add order validation to prevent negative quantities

Implements validation logic in OrderBook::add_order() to reject
orders with quantity <= 0. Includes unit tests covering edge cases.

Closes #123
```

```
perf: Optimize order matching algorithm using binary search

Replaced linear search with binary search in price level lookup,
reducing average latency from 15μs to 3μs in benchmarks.
```

**Bad Examples**:
```
fixed stuff
update code
changes
```

### Branch Naming

Use descriptive branch names:

```
feature/order-validation
fix/memory-leak-order-book
perf/optimize-matching-engine
docs/update-api-documentation
```

### Pull Request Guidelines

- Keep PRs **focused and small** (< 500 lines ideally)
- Include **unit tests** for new functionality
- Ensure **all tests pass** before requesting review
- Update **documentation** if APIs change
- Add **benchmark results** for performance changes

### Do Not Commit

Never commit:
- Binary files (executables, `.o`, `.obj`)
- IDE-specific files (`.vscode`, `.idea`) unless project-standard
- Secrets or credentials
- Large data files
- Generated files (build artifacts)

Use `.gitignore`:

```
# Build artifacts
build/
*.o
*.obj
*.exe

# IDE
.vscode/
.idea/

# Dependencies
third_party/

# Test artifacts
*.test
*.gcov
```

---

## Code Review Process

### Review Checklist

Reviewers must verify:

- [ ] **Correctness**: Does the code work as intended?
- [ ] **Testing**: Are there sufficient unit tests?
- [ ] **Performance**: No unnecessary allocations or copies in hot paths?
- [ ] **Style**: Follows coding standards?
- [ ] **Documentation**: Public APIs documented?
- [ ] **Security**: Input validation, no buffer overflows?
- [ ] **Error Handling**: Appropriate error handling?
- [ ] **Simplicity**: Is the code as simple as possible?

### Providing Feedback

- Be **constructive and respectful**
- Explain **why** changes are needed
- Suggest **specific improvements**
- Distinguish between **blocking issues** and **suggestions**

**Good Feedback**:
```
This function allocates a vector on every call. For HFT, consider
making m_buffer a member variable and reusing it to avoid allocation
overhead. Example: [link to similar code]
```

**Bad Feedback**:
```
This is slow, fix it.
```

### Responding to Feedback

- **Address all comments** or explain why not
- Ask for **clarification** if feedback is unclear
- Be **open to suggestions**
- **Update the PR** based on feedback
- Mark conversations as **resolved** after addressing

### Approval Requirements

- Minimum **2 approvals** from team members
- **All CI checks passing** (tests, linting, static analysis)
- **No unresolved conversations**
- **Documentation updated** if needed

---

## Build System

### CMake Standards

Use modern CMake practices (3.26+):

```cmake
cmake_minimum_required(VERSION 3.26)
project(StringUtils VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  add_compile_options(-O3 -march=native) # HFT optimizations
elseif(MSVC)
  add_compile_options(/W4 /WX)
  add_compile_options(/O2)
endif()

# Library target
add_library(string_utils STATIC
  src/trim.cpp
  src/split.cpp
)

target_include_directories(string_utils PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Tests
enable_testing()
add_subdirectory(tests)
```

### Compiler Warnings

Enable maximum warning levels and treat warnings as errors:

```cmake
# GCC/Clang
-Wall -Wextra -Wpedantic -Werror

# MSVC
/W4 /WX
```

### Build Types

Support standard build configurations:

- **Debug**: No optimization, full debug symbols (`-O0 -g`)
- **Release**: Full optimization, no debug info (`-O3 -DNDEBUG`)
- **RelWithDebInfo**: Optimized with debug symbols (`-O2 -g`)

---

## Class Implementation Best Practices

### Inline Implementations Outside Class

Keep class declarations clean by implementing methods outside the class body:

**Good Example**:

```cpp
template<typename T>
class matrixX_t
{
public:
  matrixX_t();
  size_t num_rows() const { return m_rows; }
  size_t num_cols() const { return m_cols; }
  
  vectorX_t<T> get_col(size_t c) const;

private:
  std::vector<T> m_data;
  size_t m_rows = 0;
  size_t m_cols = 0;
};

/**
 * @brief Get a column vector from the matrix
 * @param c Column index (zero-based)
 * @return Column vector at index c
 */
template<typename T>
inline vectorX_t<T> matrixX_t<T>::get_col(size_t c) const
{
  vectorX_t<T> col_vec(m_rows);
  for (size_t i = 0; i < m_rows; ++i)
  {
    col_vec(i) = (*this)(i, c);
  }
  return col_vec;
}
```

**Rationale**: This approach keeps class declarations focused on the interface, making it easier to understand the object's structure at a glance. Single-line getters and trivial methods can remain inline within the class for convenience.

---

## Markdown Documentation

### File Organization

- Keep markdown files under **~1,200 lines** for maintainability
- Split large documents into multiple files with clear naming
- Use consistent heading hierarchy

### Document Structure

Include a table of contents for files > 200 lines:

```markdown
# Document Title

## Table of Contents
- [Section 1](#section-1)
- [Section 2](#section-2)

## Section 1

Content here...

## Section 2

More content...
```

### Code Examples

Always specify language for syntax highlighting:

````markdown
```cpp
int main() {
  return 0;
}
```
````

### Links

Use relative links for internal documentation:

```markdown
See [Architecture Document](./Project_ARCHITECTURE.md) for details.
```

---

## Enforcement and Continuous Improvement

### Automated Checks

- **Linting**: Run `clang-format` in CI pipeline
- **Static Analysis**: Use `clang-tidy`, `cppcheck`
- **Unit Tests**: All tests must pass before merge
- **Code Coverage**: Enforce tiered requirements (see [Testing Standards](#testing-standards))

### Regular Reviews

- Review and update standards **quarterly**
- Incorporate lessons learned from production issues
- Solicit feedback from team members
- Update examples with real code from the project

### Tool Configuration

Maintain configuration files in repository:
- `.clang-format` - Code formatting rules
- `.clang-tidy` - Static analysis rules
- `.gitignore` - Version control exclusions
- `CMakeLists.txt` - Build configuration

---

## References and Resources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Effective Modern C++ by Scott Meyers](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)
- [CMake Documentation](https://cmake.org/documentation/)
- [Google Test Framework](https://github.com/google/googletest)

---

Last Updated: 2025-12-13
