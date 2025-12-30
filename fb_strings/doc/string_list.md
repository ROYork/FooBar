# string_list Class Reference

The `fb::string_list` class is a container for lists of strings with powerful manipulation methods. It wraps `std::vector<std::string>` and provides convenience methods inspired by Python and modern C++ idioms.

## Table of Contents
- [Overview](#overview)
- [Construction](#construction)
- [Element Access](#element-access)
- [Iterators](#iterators)
- [Capacity](#capacity)
- [Modifiers](#modifiers)
- [String List Operations](#string-list-operations)
- [Search Operations](#search-operations)
- [Sorting and Ordering](#sorting-and-ordering)
- [Utility Operations](#utility-operations)
- [Conversion](#conversion)
- [Operators](#operators)

---

## Overview

```cpp
#include <fb/string_list.h>

fb::string_list list{"apple", "banana", "cherry"};
std::string joined = list.join(", ");  // "apple, banana, cherry"

auto filtered = list.filter_starts_with("a");  // {"apple"}
list.sort().reverse();  // Method chaining
```

Key features:
- Full STL compatibility (iterators, range-based for)
- Python-like convenience methods
- Seamless integration with `std::vector<std::string>`
- Method chaining for fluent interface

---

## Construction

### Default Constructor

```cpp
fb::string_list list;  // Empty list
```

### Initializer List

```cpp
fb::string_list list{"a", "b", "c"};
```

### From Vector

```cpp
std::vector<std::string> vec{"hello", "world"};
fb::string_list list(vec);              // Copy
fb::string_list list(std::move(vec));   // Move
```

### From Iterator Range

```cpp
std::vector<std::string> vec{"a", "b", "c"};
fb::string_list list(vec.begin(), vec.end());
```

### Factory Methods

```cpp
// Split a string
auto list = fb::string_list::from_split("a,b,c", ',');
auto list = fb::string_list::from_split("a::b::c", "::");
auto list = fb::string_list::from_split("a,,b", ',', false);  // Skip empty

// Split into lines
auto lines = fb::string_list::from_lines("line1\nline2\nline3");
```

---

## Element Access

### `at(index)`

Bounds-checked element access.

```cpp
list.at(0);       // First element (throws if empty)
list.at(100);     // Throws std::out_of_range
```

### `operator[]`

Unchecked element access.

```cpp
list[0] = "new value";
```

### `front()` / `back()`

```cpp
list.front();  // First element
list.back();   // Last element
```

### `data()`

```cpp
std::string* ptr = list.data();
```

---

## Iterators

Full STL-compatible iterators:

```cpp
// Range-based for
for (const auto& s : list) { ... }

// Standard iterators
for (auto it = list.begin(); it != list.end(); ++it) { ... }

// Reverse iterators
for (auto it = list.rbegin(); it != list.rend(); ++it) { ... }

// Const iterators
list.cbegin(), list.cend(), list.crbegin(), list.crend()
```

---

## Capacity

```cpp
list.empty();       // bool - is list empty?
list.size();        // size_t - number of elements
list.max_size();    // Maximum possible size
list.reserve(100);  // Reserve capacity
list.capacity();    // Current capacity
list.shrink_to_fit();
```

---

## Modifiers

```cpp
list.clear();                    // Remove all elements
list.insert(it, "value");        // Insert at position
list.erase(it);                  // Remove at position
list.erase(first, last);         // Remove range
list.push_back("value");         // Add to end
list.emplace_back("value");      // Construct in place
list.pop_back();                 // Remove last
list.resize(10);                 // Resize
list.resize(10, "default");      // Resize with default
list.swap(other);                // Swap contents
```

---

## String List Operations

### `join(delimiter)`

Join all strings with a delimiter.

```cpp
fb::string_list list{"a", "b", "c"};
list.join(", ");   // "a, b, c"
list.join(',');    // "a,b,c"
list.join();       // "abc" (no delimiter)
```

### `filter(predicate)`

Filter elements by predicate.

```cpp
auto filtered = list.filter([](const std::string& s) {
  return s.size() > 3;
});
```

### `map(transform)`

Transform elements.

```cpp
auto upper = list.map([](const std::string& s) {
  return fb::to_upper(s);
});
```

### `filter_containing(substr)`

Filter elements containing a substring.

```cpp
fb::string_list files{"readme.md", "main.cpp", "test.cpp"};
auto cpp_files = files.filter_containing(".cpp");
// {"main.cpp", "test.cpp"}
```

### `filter_starts_with(prefix)` / `filter_ends_with(suffix)`

```cpp
files.filter_starts_with("test");
files.filter_ends_with(".md");
```

### `filter_matching(regex)`

Filter by regex pattern.

```cpp
files.filter_matching("^test.*\\.cpp$");
```

---

## Search Operations

### `contains(str)`

Check if list contains a string.

```cpp
list.contains("hello");              // Exact match
list.contains_ignore_case("HELLO");  // Case-insensitive
```

### `index_of(str)` / `last_index_of(str)`

Find position of a string.

```cpp
auto idx = list.index_of("hello");       // std::optional<size_t>
auto idx = list.last_index_of("hello");  // Last occurrence
```

### `count(str)`

Count occurrences.

```cpp
size_t n = list.count("hello");
```

---

## Sorting and Ordering

All sorting methods return `*this` for chaining.

### `sort()`

Sort in ascending order.

```cpp
list.sort();
```

### `sort(comparator)`

Sort with custom comparator.

```cpp
list.sort([](const std::string& a, const std::string& b) {
  return a.size() < b.size();
});
```

### `sort_ignore_case()`

Case-insensitive sort.

```cpp
fb::string_list list{"Banana", "apple", "Cherry"};
list.sort_ignore_case();  // {"apple", "Banana", "Cherry"}
```

### `sort_natural()`

Natural sort (handles embedded numbers).

```cpp
fb::string_list files{"file10", "file2", "file1"};
files.sort_natural();  // {"file1", "file2", "file10"}
```

### `reverse()`

Reverse order.

```cpp
list.reverse();
```

### `unique()`

Remove consecutive duplicates.

```cpp
list.sort().unique();  // Sort first for full dedup
```

### `shuffle()`

Randomly shuffle elements.

```cpp
list.shuffle();
```

---

## Utility Operations

### `remove_empty()` / `remove_blank()`

```cpp
list.remove_empty();  // Remove empty strings
list.remove_blank();  // Remove empty or whitespace-only
```

### `trim_all()`

Trim whitespace from all elements.

```cpp
fb::string_list list{"  hello  ", "  world  "};
list.trim_all();  // {"hello", "world"}
```

### `to_lower_all()` / `to_upper_all()`

```cpp
list.to_lower_all();
list.to_upper_all();
```

### `remove_duplicates()`

Remove duplicate strings (preserves order).

```cpp
fb::string_list list{"a", "b", "a", "c", "b"};
list.remove_duplicates();  // {"a", "b", "c"}
```

### `take(n)` / `skip(n)` / `take_last(n)`

```cpp
list.take(3);       // First 3 elements
list.skip(3);       // All except first 3
list.take_last(3);  // Last 3 elements
```

### `slice(start, end)`

Get a range of elements.

```cpp
list.slice(1, 4);  // Elements [1, 4)
```

---

## Conversion

### `to_vector()`

Get reference to underlying vector.

```cpp
std::vector<std::string>& vec = list.to_vector();
```

### `to_vector_copy()`

Get a copy of the underlying vector.

```cpp
std::vector<std::string> copy = list.to_vector_copy();
```

---

## Operators

### Equality

```cpp
list1 == list2
list1 != list2
```

### Concatenation

```cpp
auto combined = list1 + list2;
list1 += list2;
list1 += "new element";
```

### Stream Operator

```cpp
fb::string_list list;
list << "a" << "b" << "c";
```

---

## Method Chaining Example

```cpp
fb::string_list raw{
  "  Apple  ",
  "  BANANA  ",
  "",
  "  cherry  ",
  "  apple  "
};

auto result = raw
  .trim_all()
  .remove_empty()
  .to_lower_all()
  .remove_duplicates()
  .sort()
  .join(", ");

// result: "apple, banana, cherry"
```

---

## Comparison with std::vector<std::string>

| Feature | string_list | std::vector<std::string> |
|---------|-------------|-----------------------------|
| STL interface | Yes | Yes |
| String filtering | Yes | No (manual loop) |
| Join operation | Yes | No |
| Trim all strings | Yes | No |
| Remove duplicates | Yes | No (manual) |
| Case-insensitive ops | Yes | No |
| Natural sort | Yes | No |
| Method chaining | Yes | No |

---

## What's NOT Implemented

- **Regex filtering inline**: Use `filter()` with `std::regex_match`
- **In-place filter**: `filter()` returns a new list
- **Parallel operations**: All operations are single-threaded

---

## See Also

- [string_utils.md](string_utils.md) - String manipulation functions
- [iterators.md](iterators.md) - Line/token iterators
- [format.md](format.md) - String formatting

