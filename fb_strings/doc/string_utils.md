# String Utilities API Reference

This document describes the string utility functions in the `fb` namespace.

## Table of Contents
- [Trimming Functions](#trimming-functions)
- [Case Conversion](#case-conversion)
- [Prefix and Suffix Operations](#prefix-and-suffix-operations)
- [Searching](#searching)
- [Replacement](#replacement)
- [Removal](#removal)
- [Splitting and Joining](#splitting-and-joining)
- [Substring Extraction](#substring-extraction)
- [Padding and Alignment](#padding-and-alignment)
- [Validation Functions](#validation-functions)
- [Number Parsing](#number-parsing)
- [Number Conversion](#number-conversion)
- [Encoding and Decoding](#encoding-and-decoding)
- [Comparison](#comparison)
- [Miscellaneous](#miscellaneous)

---

## Trimming Functions

### `trim`

Remove leading and trailing whitespace from a string.

```cpp
std::string trim(std::string_view str);
std::string trim(std::string_view str, std::string_view chars);
```

**Examples:**
```cpp
fb::trim("  hello  ");      // "hello"
fb::trim("xxhellox", "x");  // "hello"
```

### `trim_left` / `trim_right`

Remove whitespace or specified characters from one side only.

```cpp
fb::trim_left("  hello  ");   // "hello  "
fb::trim_right("  hello  ");  // "  hello"
```

---

## Case Conversion

All case conversion functions are ASCII-only for predictable performance.

### `to_upper` / `to_lower`

```cpp
fb::to_upper("Hello");  // "HELLO"
fb::to_lower("Hello");  // "hello"
```

### `capitalize`

Capitalize first character, lowercase the rest.

```cpp
fb::capitalize("hELLO");  // "Hello"
```

### `title_case`

Capitalize first character of each word.

```cpp
fb::title_case("hello world");  // "Hello World"
```

### `swap_case`

Swap case of all characters.

```cpp
fb::swap_case("Hello");  // "hELLO"
```

---

## Prefix and Suffix Operations

### `starts_with` / `ends_with`

Check if string starts or ends with a prefix/suffix.

```cpp
fb::starts_with("hello", "hel");   // true
fb::ends_with("hello.txt", ".txt"); // true
```

### `remove_prefix` / `remove_suffix`

Remove prefix/suffix if present.

```cpp
fb::remove_prefix("hello world", "hello ");  // "world"
fb::remove_suffix("file.txt", ".txt");       // "file"
```

### `ensure_prefix` / `ensure_suffix`

Add prefix/suffix if not already present.

```cpp
fb::ensure_prefix("world", "hello ");  // "hello world"
fb::ensure_suffix("file", ".txt");     // "file.txt"
```

---

## Searching

### `contains`

Check if string contains a substring or character.

```cpp
fb::contains("hello world", "lo wo");  // true
fb::contains("hello", 'e');            // true
```

### `count`

Count occurrences of a substring or character.

```cpp
fb::count("abababab", "ab");  // 4
fb::count("hello", 'l');      // 2
```

### `find_any` / `find_any_last`

Find first/last occurrence of any character from a set.

```cpp
fb::find_any("hello", "aeiou");       // 1 (position of 'e')
fb::find_any_last("hello", "aeiou");  // 4 (position of 'o')
```

---

## Replacement

### `replace` / `replace_all`

Replace first or all occurrences of a substring.

```cpp
fb::replace("hello hello", "hello", "hi");      // "hi hello"
fb::replace_all("hello hello", "hello", "hi");  // "hi hi"
```

---

## Removal

### `remove` / `remove_all`

Remove first or all occurrences of a substring or character.

```cpp
fb::remove("hello", 'l');      // "helo"
fb::remove_all("hello", 'l');  // "heo"
```

---

## Splitting and Joining

### `split`

Split string by delimiter.

```cpp
auto parts = fb::split("a,b,c", ',');           // {"a", "b", "c"}
auto parts = fb::split("a::b::c", "::");        // {"a", "b", "c"}
auto parts = fb::split("a,,b", ',', false);     // {"a", "b"} (skip empty)
```

### `split_lines`

Split string into lines (handles CR, LF, CRLF).

```cpp
auto lines = fb::split_lines("line1\nline2\r\nline3");
```

### `split_whitespace`

Split on any whitespace.

```cpp
auto words = fb::split_whitespace("  hello   world  ");  // {"hello", "world"}
```

### `split_n`

Split into at most n parts.

```cpp
auto parts = fb::split_n("a,b,c,d", ',', 2);  // {"a", "b,c,d"}
```

### `join`

Join strings with a delimiter.

```cpp
std::vector<std::string> v = {"a", "b", "c"};
fb::join(v, ", ");  // "a, b, c"
```

---

## Substring Extraction

### `left` / `right` / `mid`

Extract substrings.

```cpp
fb::left("hello", 2);      // "he"
fb::right("hello", 2);     // "lo"
fb::mid("hello", 1, 3);    // "ell"
```

### `slice`

Python-style slicing with negative index support.

```cpp
fb::slice("hello", 1, 4);   // "ell"
fb::slice("hello", -3);     // "llo"
fb::slice("hello", 0, -1);  // "hell"
fb::slice("hello", -3, -1); // "ll"
```

### `truncate`

Truncate with optional ellipsis.

```cpp
fb::truncate("hello world", 8);  // "hello..."
```

---

## Padding and Alignment

### `pad_left` / `pad_right` / `center`

Pad strings to a target width.

```cpp
fb::pad_left("42", 5, '0');   // "00042"
fb::pad_right("hi", 5);       // "hi   "
fb::center("hi", 6);          // "  hi  "
```

### `repeat`

Repeat a string n times.

```cpp
fb::repeat("ab", 3);  // "ababab"
```

---

## Validation Functions

### `is_empty` / `is_blank`

```cpp
fb::is_empty("");       // true
fb::is_blank("   ");    // true (whitespace-only)
```

### `is_numeric` / `is_integer` / `is_float`

```cpp
fb::is_numeric("123");   // true (digits only)
fb::is_integer("-123");  // true (optional sign + digits)
fb::is_float("3.14e10"); // true
```

### `is_alpha` / `is_alphanumeric`

```cpp
fb::is_alpha("Hello");        // true
fb::is_alphanumeric("abc123"); // true
```

### `is_hex` / `is_binary` / `is_octal`

```cpp
fb::is_hex("0xff");    // true
fb::is_binary("1010"); // true
```

### `is_ascii` / `is_printable`

```cpp
fb::is_ascii("hello");       // true
fb::is_printable("hello\n"); // false (contains newline)
```

### `is_identifier`

Check if string is a valid C identifier.

```cpp
fb::is_identifier("my_var");   // true
fb::is_identifier("123var");   // false
```

---

## Number Parsing

All parsing functions return `std::optional` for safe error handling.

### `to_int` / `to_long` / `to_llong`

```cpp
auto val = fb::to_int("123");      // std::optional<int>(123)
auto val = fb::to_int("abc");      // std::nullopt
auto val = fb::to_int("ff", 16);   // std::optional<int>(255)
```

### `to_double` / `to_float`

```cpp
auto val = fb::to_double("3.14");  // std::optional<double>(3.14)
```

---

## Number Conversion

### `from_int` / `from_long` / `from_double`

```cpp
fb::from_int(255);           // "255"
fb::from_int(255, 16);       // "ff"
fb::from_double(3.14159, 2); // "3.1"
```

---

## Encoding and Decoding

### `to_hex` / `from_hex`

```cpp
fb::to_hex("Hello");         // "48656c6c6f"
fb::from_hex("48656c6c6f");  // std::optional("Hello")
```

### `to_base64` / `from_base64`

```cpp
fb::to_base64("Hello");      // "SGVsbG8="
fb::from_base64("SGVsbG8="); // std::optional("Hello")
```

### `url_encode` / `url_decode`

```cpp
fb::url_encode("a=b&c=d");     // "a%3Db%26c%3Dd"
fb::url_decode("Hello%20World"); // std::optional("Hello World")
```

### `html_escape` / `html_unescape`

```cpp
fb::html_escape("<div>");    // "&lt;div&gt;"
fb::html_unescape("&amp;");  // "&"
```

### `json_escape` / `json_unescape`

```cpp
fb::json_escape("hello\n");         // "hello\\n"
fb::json_unescape("hello\\nworld"); // std::optional("hello\nworld")
```

---

## Comparison

### `compare_ignore_case` / `equals_ignore_case`

Case-insensitive comparison (ASCII-only).

```cpp
fb::equals_ignore_case("Hello", "HELLO");  // true
fb::compare_ignore_case("abc", "DEF");     // < 0
```

### `natural_compare`

Natural sort comparison (handles embedded numbers).

```cpp
fb::natural_compare("file2", "file10");  // < 0
```

---

## Miscellaneous

### `reverse`

```cpp
fb::reverse("hello");  // "olleh"
```

### `squeeze`

Remove consecutive duplicate characters.

```cpp
fb::squeeze("aabbcc");        // "abc"
fb::squeeze("hello  world", ' '); // "hello world"
```

### `normalize_whitespace`

Collapse whitespace to single spaces.

```cpp
fb::normalize_whitespace("  hello   world  ");  // "hello world"
```

### `word_wrap`

Wrap text at specified width.

```cpp
fb::word_wrap("hello world foo bar", 10);
```

### `indent` / `dedent`

Add/remove indentation.

```cpp
fb::indent("hello\nworld", "  ");  // "  hello\n  world"
```

### `common_prefix` / `common_suffix`

```cpp
fb::common_prefix("hello", "help");  // "hel"
fb::common_suffix("hello", "jello"); // "ello"
```

### `levenshtein_distance`

Calculate edit distance between strings.

```cpp
fb::levenshtein_distance("kitten", "sitting");  // 3
```
