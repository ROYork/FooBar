# CSV Parser - RFC 4180 Compliant Parser

## Overview

`fb::csv_parser` is an RFC 4180 compliant CSV parser with configurable options for handling real-world CSV variations. It provides random access to parsed data and supports iteration.

**Key Features:**

- RFC 4180 compliance with extensions
- Header row support with column name access
- Configurable delimiter and quote characters
- Whitespace trimming options
- UTF-8 validation
- Strict and lenient parsing modes
- Iterator support for range-based for

## Quick Start

```cpp
#include <fb/csv_parser.h>

// Parse a CSV file with headers
fb::csv_parser parser("data.csv");

// Access by column name
for (size_t row = 0; row < parser.row_count(); ++row) {
    std::cout << parser.get(row, "name") << std::endl;
}

// Iterate over rows
for (const auto& row : parser) {
    std::cout << row[0] << ", " << row[1] << std::endl;
}
```

---

## Constructors

```cpp
csv_parser(const std::string& filename);                         // From file
csv_parser(const std::string& filename, const csv_config& cfg);  // With config
csv_parser(std::istream& stream);                                // From stream
csv_parser(std::istream& stream, const csv_config& cfg);         // With config
```

---

## Configuration

```cpp
fb::csv_config config;
config.has_header = true;         // First row is header (default: true)
config.strict_mode = true;        // Enforce RFC 4180 (default: true)
config.trim_whitespace = false;   // Trim field whitespace (default: false)
config.validate_utf8 = true;      // Validate UTF-8 (default: true)
config.delimiter = ',';           // Field delimiter (default: ',')

fb::csv_parser parser("data.csv", config);
```

---

## Data Access

### By Index

```cpp
std::string value = parser.get(row, column);  // Get field
std::string value = parser[row][column];      // Operator access
```

### By Column Name

```cpp
std::string name = parser.get(row, "name");   // By header name
int col = parser.column_index("name");        // Get column index
```

### Row Count/Column Count

```cpp
size_t rows = parser.row_count();
size_t cols = parser.column_count();
```

---

## Iteration

```cpp
// Iterate over rows
for (const auto& row : parser) {
    for (const auto& field : row) {
        std::cout << field << " ";
    }
    std::cout << std::endl;
}
```

---

## Error Handling

The parser throws `std::runtime_error` for:
- File not found
- Invalid CSV format (in strict mode)
- Mismatched quotes
- Invalid UTF-8 (if validation enabled)

```cpp
try {
    fb::csv_parser parser("file.csv");
} catch (const std::runtime_error& e) {
    std::cerr << "Parse error: " << e.what() << std::endl;
}
```

---

## Example: Data Processing

```cpp
fb::csv_config config;
config.has_header = true;
config.trim_whitespace = true;

fb::csv_parser parser("sales.csv", config);

double total = 0;
for (size_t i = 0; i < parser.row_count(); ++i) {
    std::string amount_str = parser.get(i, "amount");
    total += std::stod(amount_str);
}
std::cout << "Total: " << total << std::endl;
```

---

## Comparison with Other Parsers

| Feature | fb::csv_parser | Manual parsing |
|---------|----------------|----------------|
| RFC 4180 compliant | Yes | Manual |
| Quoted field support | Yes | Complex |
| Header mapping | Yes | Manual |
| Memory efficiency | Good | Varies |

---

## What's NOT Implemented

- **Streaming/incremental parsing**: Entire file loaded into memory
- **Writing CSV**: Read-only parser
- **Type conversion**: Fields returned as strings

---

## See Also

- [index.md](index.md) - Library overview
- [circular_buffer.md](circular_buffer.md) - Circular buffer
