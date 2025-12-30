# Hashing - String Hash Functions

## Overview

The `string_hash.h` module provides fast, non-cryptographic hash functions for string hashing. These are suitable for hash tables, caching, and data integrity checks, but NOT for security-critical applications.

**Key Features:**

- FNV-1a (32-bit and 64-bit): Fast, good distribution
- DJB2: Simple, widely used
- Case-insensitive hash for lookups (ASCII only)
- CRC-32: IEEE polynomial for checksums

## Quick Start

```cpp
#include <fb/string_hash.h>

std::string key = "hello";
uint32_t hash = fb::hash_fnv1a_32(key);

// Case-insensitive lookup
uint32_t h1 = fb::hash_case_insensitive("Hello");
uint32_t h2 = fb::hash_case_insensitive("HELLO");
// h1 == h2
```

---

## Hash Functions

| Function | Description |
|----------|-------------|
| `hash_fnv1a_32(str)` | FNV-1a 32-bit hash |
| `hash_fnv1a_64(str)` | FNV-1a 64-bit hash |
| `hash_djb2(str)` | DJB2 hash (32-bit) |
| `hash_case_insensitive(str)` | Case-insensitive FNV-1a (ASCII) |

```cpp
fb::hash_fnv1a_32("hello");       // 32-bit hash
fb::hash_fnv1a_64("hello");       // 64-bit hash (fewer collisions)
fb::hash_djb2("hello");           // DJB2 algorithm
```

---

## CRC-32

For data integrity checks.

```cpp
uint32_t crc = fb::crc32("data to checksum");

// Binary data
const uint8_t data[] = {0x01, 0x02, 0x03};
uint32_t crc = fb::crc32(data, sizeof(data));
```

---

## Case-Insensitive Hashing

For case-insensitive hash table lookups:

```cpp
std::unordered_map<std::string, int, 
    decltype([](const std::string& s) { return fb::hash_case_insensitive(s); })
> map;
```

---

## Comparison with std

| Feature | fb_strings | C++ std |
|---------|------------|---------|
| FNV-1a | Yes | No |
| DJB2 | Yes | No |
| CRC-32 | Yes | No |
| Case-insensitive hash | Yes | No |
| std::hash integration | No | Yes |

---

## What's NOT Implemented

- **Cryptographic hashes**: Use OpenSSL/libsodium for SHA, MD5, etc.
- **std::hash specialization**: Use directly with custom hash functor

---

## See Also

- [string_utils.md](string_utils.md) - String utilities
- [encoding.md](encoding.md) - Encoding functions
