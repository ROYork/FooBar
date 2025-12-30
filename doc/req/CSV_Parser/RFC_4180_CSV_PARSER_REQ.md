# An RFC 4180 Compliant CSV Parser class

This file describes the requirements for adding a RFC 4180 Compliant CSV parsing class to a C++ utility library.

## Project Brief

- **Class name**: `csv_parser`
- **Use CMAKE** for build system
- **Language**: C++
- **Dependencies**: C++ standard library only
- **IMPORTANT**: Follow the Coding standards in [Coding_Standards.md](Coding_Standards.md)

# Top Level Requirements

## CSV Format Compliance

[CSV-REQ-001] **RFC 4180 Compliance** The `csv_parser` shall be compliant with RFC 4180 (Common Format and MIME Type for Comma-Separated Values (CSV) Files) with the following specifications:
- Fields containing line breaks (CRLF or LF), double quotes, or commas must be enclosed in double quotes
- A double quote appearing inside a field must be escaped by preceding it with another double quote
- Fields may or may not be enclosed in double quotes
- The last field in a record may or may not be followed by a line break
- Each record should contain the same number of fields

[CSV-REQ-002] **UTF-8 BOM Support** The `csv_parser` shall detect and properly handle UTF-8 Byte Order Mark (BOM: 0xEF 0xBB 0xBF) at the beginning of CSV files.
- The BOM shall be recognized only at the start of the file or stream; a BOM encountered elsewhere shall be treated as data.

[CSV-REQ-003] **Line Ending Support** The `csv_parser` shall support both:
- CRLF (`\r\n`) - Windows-style line endings
- LF (`\n`) - Unix/Linux-style line endings
- Mixed line endings within the same file

[CSV-REQ-004] **Multi-line Field Support** The `csv_parser` shall correctly parse fields that contain literal newlines when enclosed in double quotes. The newlines shall be preserved in the returned field value.

Example:
```csv
"12","Field_Name","Type","Unit","uint8","1","Multi-line documentation:
Line 2 of documentation,
Line 3 with comma, and
Final line"
```
The last field should be returned as a string containing preserved line ending characters as they appear in the input.

[CSV-REQ-004A] **Line Ending Preservation** The `csv_parser` shall preserve line endings exactly as they appear in the input when they occur inside quoted fields:
- CRLF (`\r\n`) shall be preserved as `\r\n`
- LF (`\n`) shall be preserved as `\n`
- Mixed line endings shall be preserved as-is

[CSV-REQ-005] **Quote Escaping** The `csv_parser` shall handle escaped double quotes within quoted fields according to RFC 4180 (doubled quotes).

Example: The field `"He said ""Hello"" to me"` should be parsed as: `He said "Hello" to me`

[CSV-REQ-005A] **Non-Strict Quote Handling** When `strict_mode = false`, the `csv_parser` shall treat double quotes as follows:
- A field is considered quoted only if its first character is a double quote (`"`). If `trim_whitespace = true`, the first non-whitespace character is used for this determination.
- Double quotes appearing in unquoted fields are treated as literal characters and do not trigger quoted-field parsing.

[CSV-REQ-005B] **Strict Quote Grammar** When `strict_mode = true`, the `csv_parser` shall enforce RFC 4180 quoted-field grammar:
- Double quotes may appear only as the first and last character of a quoted field or as escaped doubled quotes inside a quoted field.
- If a field is unquoted, any double quote character is an error.
- After a closing quote, the next character must be a field delimiter or a record terminator; optional whitespace is permitted between the closing quote and the delimiter/terminator and shall be ignored.

[CSV-REQ-006] **Empty Fields** The `csv_parser` shall correctly handle empty fields.

Examples:
- `"field1","","field3"` - second field is empty string
- `field1,,field3` - second field is empty string
- `"field1",,"field3"` - second field is empty string

[CSV-REQ-007] **Comma Handling** The `csv_parser` shall treat commas as field delimiters only when outside of double-quoted fields.

Example: `"Smith, John",25,"Engineer, Senior"` has 3 fields, not 5.

[CSV-REQ-008] **Whitespace Handling** The `csv_parser` shall handle whitespace as follows:
- Whitespace outside quotes: Configurable option to trim or preserve (default: preserve)
- Whitespace inside quotes: Always preserved
- Example with trim enabled: `field1 , field2` → `["field1", "field2"]`
- Example with trim disabled: `field1 , field2` → `["field1 ", " field2"]`
- Whitespace trimming applies only to unquoted fields. Whitespace outside a quoted field is treated as part of the unquoted field and is trimmed only when `trim_whitespace = true`.

## Class Interface Requirements

[CSV-REQ-009] **Construction from File** The `csv_parser` shall provide a constructor that accepts a file path:
```cpp
csv_parser(const std::string& filename);
csv_parser(const std::filesystem::path& filepath);
```

[CSV-REQ-010] **Construction from Stream** The `csv_parser` shall provide a constructor that accepts an input stream:
```cpp
csv_parser(std::istream& stream);
```

[CSV-REQ-011] **Parsing Trigger** The `csv_parser` shall parse the CSV data upon construction or provide an explicit `parse()` method. The implementor shall choose the most appropriate design.

[CSV-REQ-012] **Table Representation** The `csv_parser` shall represent the CSV file as a two-dimensional table with rows and columns.

[CSV-REQ-013] **Row Count** The `csv_parser` shall provide a method to get the number of rows:
```cpp
size_t row_count() const;
```
- When `has_headers = true` and a header row is present, `row_count()` shall exclude the header row and count only data rows.

[CSV-REQ-014] **Column Count** The `csv_parser` shall provide a method to get the number of columns:
```cpp
size_t column_count() const;
```
- When `has_headers = true` and a header row is present, `column_count()` shall return the number of header fields.
- When `has_headers = false`, `column_count()` shall return the number of fields in the first data row.
- If no rows are present, `column_count()` shall return `0`.
- In non-strict mode, if any row contains more fields than the value defined above, `column_count()` shall return the maximum number of fields observed across all rows.

[CSV-REQ-015] **Header Detection** The `csv_parser` shall provide a constructor option to specify whether the first row contains column headers:
```cpp
csv_parser(const std::string& filename, bool has_headers = true);
```

[CSV-REQ-016] **Row Access by Index** The `csv_parser` shall provide methods to retrieve a complete row by zero-based index:
```cpp
std::vector<std::string> get_row(size_t row_index) const;
```
- Returns a `std::vector<std::string>` containing all fields in the row
- When `has_headers = true`, `row_index = 0` refers to the first data row (the header row is not addressable via `get_row()`).
- Throws `std::out_of_range` if index is invalid

[CSV-REQ-017] **Column Access by Index** The `csv_parser` shall provide methods to retrieve a complete column by zero-based index:
```cpp
std::vector<std::string> get_column(size_t col_index) const;
```
- Returns a `std::vector<std::string>` containing all fields in the column
- Excludes header row if headers are enabled
- Throws `std::out_of_range` if index is invalid

[CSV-REQ-018] **Column Access by Header** The `csv_parser` shall provide methods to retrieve a column by header name when headers are enabled:
```cpp
std::vector<std::string> get_column(const std::string& header_name) const;
```
- Returns a `std::vector<std::string>` containing all fields in the column
- Excludes the header row itself
- Throws `std::invalid_argument` if header doesn't exist or headers are not enabled

[CSV-REQ-018A] **Duplicate Headers** When headers are enabled, the `csv_parser` shall detect duplicate header names and throw `std::invalid_argument` with a descriptive message.

[CSV-REQ-019] **Cell Access** The `csv_parser` shall provide methods to retrieve individual cell values:
```cpp
std::string get_cell(size_t row, size_t col) const;
std::string get_cell(size_t row, const std::string& col_header) const;
```
- Returns the field value as a string
- When `has_headers = true`, `row = 0` refers to the first data row (the header row is not addressable via `get_cell()`).
- Throws `std::out_of_range` or `std::invalid_argument` for invalid indices/headers

[CSV-REQ-020] **Header Retrieval** The `csv_parser` shall provide a method to retrieve all column headers:
```cpp
std::vector<std::string> get_headers() const;
```
- Returns empty list if headers are not enabled

[CSV-REQ-021] **Row Iterator** The `csv_parser` should provide iterator support for rows:
```cpp
// Allow range-based for loops over rows
for (const auto& row : parser) {
    // row is std::vector<std::string>
}
```
This requirement is optional but highly recommended for ease of use.

[CSV-REQ-022] **Empty File Handling** The `csv_parser` shall handle empty CSV files gracefully:
- `row_count()` returns 0
- `column_count()` returns 0
- `get_row()`, `get_column()` throw `std::out_of_range`

[CSV-REQ-023] **Parse Validation** The `csv_parser` shall validate row field counts as follows:
- In strict mode, all rows must contain the same number of fields; the parser shall report an error if inconsistent.
- In non-strict mode, variable field counts are allowed; missing fields accessed via `get_column()` or `get_cell()` shall be returned as empty strings.

## Error Handling Requirements

[CSV-REQ-024] **File Not Found** The `csv_parser` shall throw `std::runtime_error` or `std::ios_base::failure` if the specified file cannot be opened.

[CSV-REQ-025] **Malformed CSV** The `csv_parser` shall throw `std::runtime_error` with a descriptive message if:
- A quoted field is not properly closed (missing closing quote)
- Inconsistent number of fields across rows (unless explicitly allowed by configuration)
- Invalid UTF-8 sequences are encountered (when UTF-8 validation is enabled)

[CSV-REQ-026] **Index Out of Range** The `csv_parser` shall throw `std::out_of_range` when:
- Row index exceeds `row_count() - 1`
- Column index exceeds `column_count() - 1`

[CSV-REQ-027] **Invalid Header** The `csv_parser` shall throw `std::invalid_argument` when:
- Requesting a column by header name that doesn't exist
- Requesting header operations when headers are not enabled

[CSV-REQ-028] **Error Messages** All exceptions shall include descriptive error messages indicating:
- The nature of the error
- The file name (if applicable)
- The line number where the error occurred (if applicable)

[CSV-REQ-028A] **Error Location Semantics** Error line numbers shall be 1-based and refer to the line where the error was detected. For unclosed quoted fields, the line number shall refer to the line where the quoted field started.

[CSV-REQ-029] **Logging Integration** The `csv_parser` may optionally integrate with a library logging facility for non-fatal warnings such as:
- UTF-8 BOM detected and removed
- Inconsistent whitespace detected
- Empty rows encountered

## Configuration Options

[CSV-REQ-030] **Configuration Structure** The `csv_parser` should support a configuration structure for parsing options:
```cpp
struct csv_config {
    bool has_headers = true;
    bool trim_whitespace = false;
    bool strict_mode = true;  // Require same number of fields per row
    bool validate_utf8 = true;
    char delimiter = ',';     // Optional: support other delimiters
};

csv_parser(const std::string& filename, const csv_config& config);
```

[CSV-REQ-030A] **Delimiter vs RFC 4180** RFC 4180 compliance applies only when `delimiter == ','`. Any other delimiter value is a non-RFC extension and shall be documented as such.

[CSV-REQ-031] **Whitespace Trimming** The `csv_parser` shall support optional trimming of leading/trailing whitespace from unquoted fields via configuration.

[CSV-REQ-032] **Strict Mode** The `csv_parser` shall support a strict mode that enforces:
- All rows have the same number of fields
- No empty rows
- Trailing commas are not permitted

## Performance Requirements

[CSV-REQ-033] **Memory Efficiency** The `csv_parser` should load the entire file into memory for random access. The implementation should be optimized for files up to 100MB.

[CSV-REQ-034] **Parse Speed** The `csv_parser` should be able to parse at least 50,000 rows per second on modern hardware (2GHz+ CPU).

[CSV-REQ-035] **Large File Warning** The `csv_parser` may optionally warn when parsing files larger than a configurable threshold (default: 50MB).

## Testing Requirements

[CSV-REQ-036] **Unit Tests** The `csv_parser` shall have comprehensive unit tests covering:
- RFC 4180 compliance (all edge cases)
- Empty files
- Single row files
- Single column files
- Files with only headers
- Multi-line fields
- Escaped quotes
- Empty fields
- UTF-8 BOM handling
- Mixed line endings
- All error conditions

[CSV-REQ-037] **Integration Tests** The `csv_parser` shall be tested against the normative test vectors defined in this document to ensure real-world compatibility. External fixture files are optional and must not be required to satisfy this requirement.

[CSV-REQ-038] **Performance Tests** The `csv_parser` shall include performance benchmarks measuring:
- Parse time vs file size
- Memory usage vs file size
- Access time for random cell access

## Documentation Requirements

[CSV-REQ-039] **API Documentation** All public methods shall be documented with doxygen-style comments including:
- Purpose
- Parameters
- Return values
- Exceptions thrown
- Usage examples

[CSV-REQ-040] **Usage Examples** The `csv_parser` documentation shall include complete usage examples demonstrating:
- Basic parsing with headers
- Parsing without headers
- Accessing rows and columns
- Iterating over data
- Error handling

[CSV-REQ-041] **RFC 4180 Reference** The documentation shall reference RFC 4180 compliance and any deviations from the standard.

## Normative Test Vectors

These test vectors are required. Each vector specifies an input CSV snippet, the parsing configuration, and the expected outcome.

### TV-001: Empty File
Config: `has_headers=true`, `strict_mode=true`  
Input:
```csv

```
Expected:
- `row_count()` = 0
- `column_count()` = 0
- `get_row(0)` throws `std::out_of_range`

### TV-002: Header Only
Config: `has_headers=true`, `strict_mode=true`  
Input:
```csv
Name,Age
```
Expected:
- `row_count()` = 0
- `column_count()` = 2
- `get_headers()` = `["Name","Age"]`

### TV-003: Empty Fields
Config: `has_headers=false`, `strict_mode=true`  
Input:
```csv
field1,,field3
```
Expected:
- Single row with 3 fields, middle field is empty string

### TV-004: Quoted Field with Commas and Quotes
Config: `has_headers=false`, `strict_mode=true`  
Input:
```csv
"Smith, John","He said ""Hello"""
```
Expected:
- 2 fields: `Smith, John` and `He said "Hello"`

### TV-005: Multi-line Field with Mixed Line Endings
Config: `has_headers=true`, `strict_mode=true`  
Input (CRLF between records; LF inside quoted field):
```csv
"A","B"
"Line1
Line2","C"
```
Expected:
- 1 data row, 2 columns
- First field contains `Line1\nLine2` (LF preserved inside quoted field)

### TV-006: Inconsistent Row Lengths (Strict)
Config: `has_headers=false`, `strict_mode=true`  
Input:
```csv
a,b,c
1,2
```
Expected:
- `std::runtime_error` for inconsistent field count

### TV-007: Inconsistent Row Lengths (Non-Strict)
Config: `has_headers=false`, `strict_mode=false`  
Input:
```csv
a,b,c
1,2
```
Expected:
- 2 rows accepted
- Missing field in row 2 treated as empty string when accessed

### TV-008: Random Quotes in Unquoted Field (Non-Strict)
Config: `has_headers=true`, `strict_mode=false`  
Input:
```csv
Name,Doc
Element_ID,Unique Identifier of the "partitioned" Domain element
```
Expected:
- Parse succeeds; quotes are literal characters in the Doc field

### TV-009: Random Quotes in Unquoted Field (Strict)
Config: `has_headers=true`, `strict_mode=true`  
Input:
```csv
Name,Doc
Element_ID,Unique Identifier of the "partitioned" Domain element
```
Expected:
- `std::runtime_error` due to quote in unquoted field

### TV-010: Duplicate Headers
Config: `has_headers=true`, `strict_mode=true`  
Input:
```csv
Name,Name
1,2
```
Expected:
- `std::invalid_argument` for duplicate header names

### TV-011: UTF-8 BOM at Start
Config: `has_headers=true`, `strict_mode=true`  
Input (BOM shown as UTF-8 BOM bytes):
```csv
<BOM>Name,Age
Alice,30
```
Expected:
- BOM is removed from the first header field name (`Name`)

### TV-012: Whitespace Trimming on Unquoted Fields
Config: `has_headers=false`, `strict_mode=true`, `trim_whitespace=true`  
Input:
```csv
  a ,  b  
```
Expected:
- 2 fields: `a` and `b`

### TV-013: Unclosed Quoted Field
Config: `has_headers=false`, `strict_mode=true`  
Input:
```csv
"a,b
```
Expected:
- `std::runtime_error` for missing closing quote
## RFC 4180 Compliance Notes / Deviations

The `csv_parser` targets RFC 4180 behavior when `delimiter == ','` with the following documented deviations or extensions:
- Supports LF and mixed line endings in addition to CRLF.
- Optional trimming of whitespace on unquoted fields (RFC 4180 treats spaces as data).
- Optional UTF-8 BOM detection and removal at the start of the file/stream (not defined in RFC 4180).
- Optional UTF-8 validation (controlled by `csv_config::validate_utf8`; not defined in RFC 4180).
- Optional non-strict mode allowing variable field counts per row.

## Build Integration Requirements

[CSV-REQ-042] **CMake Integration** The `csv_parser` shall integrate into the existing library CMake build system.

[CSV-REQ-043] **Dependencies** The `csv_parser` shall only depend on:
- Standard C++ standard library

[CSV-REQ-044] **Header Organization** The `csv_parser` shall be provided as:
- Header file: `csv_parser.h`
- Implementation: `csv_parser.cpp` (if needed)

## Normalization / Output Requirements

[CSV-REQ-045] **RFC 4180 Normalization Output** The `csv_parser` shall provide a method to emit a RFC 4180 compliant CSV representation of the parsed data, suitable for converting non-compliant input into compliant output.
- Required methods:
```cpp
std::string to_rfc_4180() const;
void write_rfc_4180(std::ostream& out) const;
```
- The output shall use comma (`,`) as the delimiter and CRLF (`\r\n`) as the record terminator.
- Fields shall be quoted only when required by RFC 4180 (contain comma, double quote, CR, or LF).
- Double quotes within quoted fields shall be escaped by doubling (`""`).
- The header row shall be included in the output when headers are enabled.

## Example Usage

```cpp
#include <csv_parser.h>
#include <iostream>

int main() {
    try {
        // Parse a CSV file with headers
        csv_parser parser("data.csv");

        // Get dimensions
        std::cout << "Rows: " << parser.row_count() << "\n";
        std::cout << "Columns: " << parser.column_count() << "\n";

        // Access by column name
        auto names = parser.get_column("Name");
        for (const auto& name : names) {
            std::cout << name << "\n";
        }

        // Access specific cell
        std::string value = parser.get_cell(5, "Documentation");
        std::cout << "Cell value: " << value << "\n";

        // Iterate over all rows
        for (size_t i = 0; i < parser.row_count(); ++i) {
            auto row = parser.get_row(i);
            // Process row...
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

## Future Enhancements (Out of Scope for This Version)

The following items are intentionally **out of scope** for this version and are **not required** for compliance:
- Custom delimiters beyond comma (e.g., tab, semicolon) as a fully documented feature set.
- Streaming or lazy parsing for very large files.
- Writing/modifying CSV files beyond RFC 4180 normalization output (see `to_rfc_4180()` / `write_rfc_4180()`).

## Non-Normative Implementation Notes

These notes are **informative only** and do not impose additional requirements:
- Use your best judgment for implementation details not specified here.
- A state machine is a good fit for handling RFC 4180 edge cases correctly.
- Optimize for the common case (quoted fields, CRLF line endings, consistent row lengths).

## Optional Enhancements (Non-Required)
- Column type inference and conversion utilities.
- Thread-safe access for concurrent readers.
