# Random - Random String Generation

## Overview

The `string_random.h` module provides functions for generating random strings for testing, token generation, and unique identifiers.

**Key Features:**

- Custom character set support
- Convenience functions for common patterns
- RFC-4122 UUID v4 generation
- Thread-safe (uses thread-local random engine)

## Quick Start

```cpp
#include <fb/string_random.h>

// Generate random alphanumeric string
std::string token = fb::random_alphanumeric(32);

// Generate UUID
std::string uuid = fb::random_uuid();
// e.g., "550e8400-e29b-41d4-a716-446655440000"

// Custom character set
std::string code = fb::random_string(6, "ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
```

---

## Functions

| Function | Description | Characters |
|----------|-------------|------------|
| `random_string(len, charset)` | Custom character set | User-defined |
| `random_alphanumeric(len)` | Letters and digits | A-Z, a-z, 0-9 |
| `random_alphabetic(len)` | Letters only | A-Z, a-z |
| `random_numeric(len)` | Digits only | 0-9 |
| `random_hex(len, upper)` | Hexadecimal | 0-9, a-f (or A-F) |
| `random_uuid()` | RFC-4122 UUID v4 | UUID format |

---

## Examples

### API Token

```cpp
std::string api_token = fb::random_alphanumeric(64);
```

### Verification Code

```cpp
// Easy-to-read characters (no O/0, I/1 confusion)
std::string code = fb::random_string(6, "ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
```

### Temporary Password

```cpp
std::string password = fb::random_string(12, 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%");
```

### Session ID

```cpp
std::string session_id = fb::random_hex(32);  // 128-bit random hex
```

### UUID

```cpp
std::string uuid = fb::random_uuid();
// Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
```

---

## Thread Safety

All functions are thread-safe. Each thread maintains its own random engine seeded from `std::random_device`.

---

## What's NOT Implemented

- **Cryptographically secure random**: Use OS facilities for security tokens
- **Custom random engine injection**: Fixed to use `std::mt19937`

---

## See Also

- [string_utils.md](string_utils.md) - String utilities
- [hashing.md](hashing.md) - String hashing
