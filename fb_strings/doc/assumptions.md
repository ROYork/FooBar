# Design Decisions and Assumptions

This document describes the key design decisions, assumptions, and tradeoffs made in the `fb_strings` library.

## Table of Contents
- [General Design Philosophy](#general-design-philosophy)
- [Locale Handling](#locale-handling)
- [Error Handling](#error-handling)
- [Performance Considerations](#performance-considerations)
- [API Design](#api-design)
- [Limitations](#limitations)
- [Future Considerations](#future-considerations)

---

## General Design Philosophy

### Goals

1. **HFT-Ready Performance**: Designed for high-frequency trading applications where microsecond-level latency matters.

2. **Safety First**: Use of `std::optional` for fallible operations prevents crashes from invalid input.

3. **Developer Experience**: API inspired by Python and modern C++ to be intuitive for developers.

4. **Zero External Dependencies**: The core library depends only on the C++ standard library.

### Non-Goals

1. Full Unicode support (beyond UTF-8 passthrough)
2. Locale-sensitive operations
3. Regex-heavy string manipulation (use `<regex>` directly)

---

## Locale Handling

**Decision**: ASCII-only operations for case conversion and character classification.

**Rationale**:
- Predictable performance (no locale locks or thread contention)
- Consistent behavior across platforms
- Meets requirements for typical log parsing, protocol handling, and data processing
- Avoids surprising behavior from ambient locale settings

**Impact**:
- `to_upper("cafe")` converts only ASCII letters (a-z to A-Z)
- Non-ASCII characters pass through unchanged
- No support for Turkish dotless-i or German sharp-s rules

**Workaround**: For locale-aware operations, use `<locale>` and `std::tolower`/`std::toupper` with explicit locale objects.

---

## Error Handling

### Parsing Functions

**Decision**: Return `std::optional<T>` for parsing functions (`to_int`, `to_double`, etc.).

**Rationale**:
- No exceptions on hot paths
- Caller must handle invalid input explicitly
- Aligns with modern C++ error handling patterns

**Example**:
```cpp
auto val = fb::to_int("123");
if (val) {
  use(*val);
} else {
  handle_error();
}
```

### Encoding Functions

**Decision**: Decoding functions (`from_hex`, `from_base64`, `url_decode`, `json_unescape`) return `std::optional<std::string>`.

**Rationale**:
- Invalid encoding is a data error, not a programmer error
- Allows graceful handling without try/catch

### Format Function

**Decision**: `fb::format` throws exceptions for format string errors; `fb::try_format` returns `std::optional`.

**Rationale**:
- Format string errors are typically programmer errors (caught at development time)
- Provides both throwing and non-throwing variants for flexibility

---

## Performance Considerations

### String Allocation

**Approach**:
- Use `reserve()` when output size is predictable
- Avoid intermediate string copies where possible
- Return by value to enable move semantics

**Example** (split implementation):
```cpp
std::vector<std::string> result;
result.reserve(estimated_count);  // Pre-allocate
```

### View vs Copy

**Approach**:
- Accept `std::string_view` for read-only input parameters
- Return `std::string` for output (ownership transfer)

**Rationale**:
- Avoids copying when caller has a string
- Works with string literals directly
- Clear ownership semantics

### Inline Functions

**Approach**:
- Small, frequently-called helper functions are `constexpr` or inline
- Larger functions in `.cpp` files to reduce compile times

---

## API Design

### Naming Conventions

| Convention | Example | Rationale |
|------------|---------|-----------|
| snake_case functions | `to_upper`, `split_lines` | Matches C++ standard library |
| snake_case classes | `string_list` | Utility library convention |
| Verb-based names | `trim`, `split`, `join` | Action-oriented |
| `is_` prefix for predicates | `is_empty`, `is_numeric` | Boolean nature clear |
| `to_`/`from_` for conversion | `to_int`, `from_hex` | Direction clear |

### Overloading

**Approach**: Provide overloads for common use cases.

```cpp
// Overloads for convenience
split(str, ',');           // char delimiter
split(str, "::");          // string delimiter
split(str, ',', false);    // skip empty parts
```

### Method Chaining

**Approach**: `string_list` methods return `*this` for chaining.

```cpp
list.trim_all()
    .remove_empty()
    .sort()
    .reverse();
```

---

## Limitations

### Unicode

- No Unicode normalization
- No grapheme cluster handling
- No bidirectional text support
- UTF-8 sequences pass through unchanged

**Recommendation**: For Unicode-heavy applications, consider ICU or similar libraries.

### Regular Expressions

- `string_list::filter_matching` uses `<regex>`
- No regex-based split or replace in `string_utils`

**Recommendation**: Use `<regex>` directly for complex pattern matching.

### Floating-Point Formatting

- Uses `std::ostringstream` internally
- May not match `std::format` exactly for edge cases
- No grouping separator support

### Large Strings

- No special handling for very large strings (>1GB)
- Some operations may allocate proportionally to input size

---

## Future Considerations

### Potential Additions

1. **Unicode normalization** (if demand exists)
2. **Regex-based replace** with capture groups
3. **String pooling** for memory-constrained environments
4. **Compile-time format string validation** (requires C++20)

### Breaking Changes Policy

- Major version bumps for API changes
- Deprecation warnings before removal
- Maintain backward compatibility within major versions

---

## Compatibility

### Compilers Tested

- GCC 7+ (C++17 support)
- Clang 6+ (C++17 support)
- MSVC 19.14+ (VS 2017 15.7+)

### Platform Notes

- **Windows**: Uses standard C++ only; no Windows-specific APIs
- **Linux/macOS**: Standard POSIX-compatible behavior
- **Embedded**: No dynamic allocation in core parsing functions

---

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

