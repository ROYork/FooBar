/// @file csv_parser.h
/// @brief RFC 4180 compliant CSV parser with configurable options
///
/// A CSV parser class that provides random access to parsed CSV data.
/// Supports RFC 4180 compliance with extensions for common real-world variations.
///
/// Features:
/// - RFC 4180 compliant parsing (quoted fields, escaped quotes, multi-line fields)
/// - Configurable delimiter, whitespace trimming, and strict mode
/// - UTF-8 BOM detection and removal
/// - UTF-8 validation (optional)
/// - Header row support with column access by name
/// - Iterator support for range-based for loops
/// - RFC 4180 normalized output generation
///
/// Thread Safety:
/// - The parser is NOT thread-safe for concurrent access
/// - Create separate parser instances for multi-threaded use
///
/// RFC 4180 Compliance Notes:
/// - Supports both CRLF and LF line endings (RFC 4180 specifies CRLF)
/// - Optional whitespace trimming (RFC 4180 treats spaces as data)
/// - Optional UTF-8 BOM handling (not defined in RFC 4180)
/// - Optional non-strict mode for variable field counts

#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fb
{

/// @brief Configuration options for CSV parsing
///
/// Controls parser behavior for various CSV formats and edge cases.
/// Default configuration is RFC 4180 strict mode with headers.
struct csv_config
{
  /// @brief First row contains column headers (default: true)
  ///
  /// When true, the first row is treated as headers and excluded from
  /// row_count(). Headers enable column access by name.
  bool has_headers = true;

  /// @brief Trim leading/trailing whitespace from unquoted fields (default: false)
  ///
  /// When true, whitespace is trimmed from unquoted fields only.
  /// Whitespace inside quoted fields is always preserved.
  /// Note: RFC 4180 treats spaces as data; trimming is an extension.
  bool trim_whitespace = false;

  /// @brief Enforce strict RFC 4180 compliance (default: true)
  ///
  /// When true:
  /// - All rows must have the same number of fields
  /// - Quotes in unquoted fields are errors
  /// - Empty rows are not permitted
  ///
  /// When false:
  /// - Variable field counts are allowed
  /// - Quotes in unquoted fields are treated as literal characters
  /// - Missing fields are returned as empty strings
  bool strict_mode = true;

  /// @brief Validate UTF-8 encoding (default: true)
  ///
  /// When true, invalid UTF-8 sequences cause a parsing error.
  /// When false, bytes are passed through without validation.
  bool validate_utf8 = true;

  /// @brief Field delimiter character (default: ',')
  ///
  /// Note: RFC 4180 compliance only applies when delimiter is ','.
  /// Other delimiters are supported as a non-RFC extension.
  char delimiter = ',';
};

/// @brief RFC 4180 compliant CSV parser
///
/// Parses CSV data from files or streams and provides random access to
/// the parsed data. Supports configurable parsing options for various
/// CSV formats.
///
/// Basic Usage:
/// @code
/// fb::csv_parser parser("data.csv");
///
/// std::cout << "Rows: " << parser.row_count() << std::endl;
/// std::cout << "Columns: " << parser.column_count() << std::endl;
///
/// // Access by column name (when headers enabled)
/// auto names = parser.get_column("Name");
///
/// // Access specific cell
/// std::string value = parser.get_cell(0, "Age");
/// @endcode
///
/// Configuration Example:
/// @code
/// fb::csv_config config;
/// config.has_headers = false;
/// config.trim_whitespace = true;
/// config.strict_mode = false;
/// config.delimiter = ';';
///
/// fb::csv_parser parser("data.csv", config);
/// @endcode
///
/// Iterator Example:
/// @code
/// fb::csv_parser parser("data.csv");
/// for (const auto& row : parser) {
///     for (const auto& field : row) {
///         std::cout << field << " ";
///     }
///     std::cout << std::endl;
/// }
/// @endcode
class csv_parser
{
public:
  // ============================================================================
  // Type Aliases
  // ============================================================================

  /// @brief Type representing a single row (vector of field strings)
  using row_type = std::vector<std::string>;

  /// @brief Size type for indices and counts
  using size_type = std::size_t;

  /// @brief Iterator type for row iteration
  using iterator = std::vector<row_type>::const_iterator;

  /// @brief Const iterator type for row iteration
  using const_iterator = std::vector<row_type>::const_iterator;

  // ============================================================================
  // Constructors
  // ============================================================================

  /// @brief Construct parser from file path string
  ///
  /// Parses the CSV file using default configuration (headers enabled,
  /// strict mode, no whitespace trimming).
  ///
  /// @param filename Path to the CSV file
  /// @throw std::runtime_error if file cannot be opened or contains invalid CSV
  explicit csv_parser(const std::string& filename);

  /// @brief Construct parser from file path string with custom configuration
  ///
  /// @param filename Path to the CSV file
  /// @param config Parser configuration options
  /// @throw std::runtime_error if file cannot be opened or contains invalid CSV
  csv_parser(const std::string& filename, const csv_config& config);

  /// @brief Construct parser from filesystem path
  ///
  /// Parses the CSV file using default configuration.
  ///
  /// @param filepath Path to the CSV file
  /// @throw std::runtime_error if file cannot be opened or contains invalid CSV
  explicit csv_parser(const std::filesystem::path& filepath);

  /// @brief Construct parser from filesystem path with custom configuration
  ///
  /// @param filepath Path to the CSV file
  /// @param config Parser configuration options
  /// @throw std::runtime_error if file cannot be opened or contains invalid CSV
  csv_parser(const std::filesystem::path& filepath, const csv_config& config);

  /// @brief Construct parser from input stream
  ///
  /// Parses CSV data from the provided stream using default configuration.
  /// The stream must be readable and positioned at the start of CSV data.
  ///
  /// @param stream Input stream containing CSV data
  /// @throw std::runtime_error if stream contains invalid CSV
  explicit csv_parser(std::istream& stream);

  /// @brief Construct parser from input stream with custom configuration
  ///
  /// @param stream Input stream containing CSV data
  /// @param config Parser configuration options
  /// @throw std::runtime_error if stream contains invalid CSV
  csv_parser(std::istream& stream, const csv_config& config);

  /// @brief Default destructor
  ~csv_parser() = default;

  // Non-copyable but moveable
  csv_parser(const csv_parser&)            = delete;
  csv_parser& operator=(const csv_parser&) = delete;
  csv_parser(csv_parser&&) noexcept        = default;
  csv_parser& operator=(csv_parser&&) noexcept = default;

  // ============================================================================
  // Dimension Methods
  // ============================================================================

  /// @brief Get the number of data rows
  ///
  /// Returns the count of data rows, excluding the header row when
  /// has_headers is true.
  ///
  /// @return Number of data rows
  [[nodiscard]] size_type row_count() const noexcept;

  /// @brief Get the number of columns
  ///
  /// When has_headers is true, returns the number of header fields.
  /// When has_headers is false, returns the number of fields in the first row.
  /// In non-strict mode, returns the maximum field count across all rows.
  ///
  /// @return Number of columns, or 0 if no data
  [[nodiscard]] size_type column_count() const noexcept;

  // ============================================================================
  // Data Access Methods
  // ============================================================================

  /// @brief Get a row by index
  ///
  /// Returns a copy of the row at the specified index.
  /// When has_headers is true, index 0 refers to the first data row.
  ///
  /// @param row_index Zero-based row index
  /// @return Vector of field values for the row
  /// @throw std::out_of_range if row_index >= row_count()
  [[nodiscard]] row_type get_row(size_type row_index) const;

  /// @brief Get a column by index
  ///
  /// Returns all field values for the specified column across all data rows.
  /// Does not include the header value.
  ///
  /// @param col_index Zero-based column index
  /// @return Vector of field values for the column
  /// @throw std::out_of_range if col_index >= column_count()
  [[nodiscard]] std::vector<std::string> get_column(size_type col_index) const;

  /// @brief Get a column by header name
  ///
  /// Returns all field values for the column with the specified header.
  /// Requires has_headers to be true.
  ///
  /// @param header_name Column header name
  /// @return Vector of field values for the column
  /// @throw std::invalid_argument if headers not enabled or header not found
  [[nodiscard]] std::vector<std::string> get_column(const std::string& header_name) const;

  /// @brief Get a cell value by row and column index
  ///
  /// @param row Row index (0-based, excluding header)
  /// @param col Column index (0-based)
  /// @return Cell value as string
  /// @throw std::out_of_range if indices are invalid
  [[nodiscard]] std::string get_cell(size_type row, size_type col) const;

  /// @brief Get a cell value by row index and column header
  ///
  /// @param row Row index (0-based, excluding header)
  /// @param col_header Column header name
  /// @return Cell value as string
  /// @throw std::out_of_range if row index is invalid
  /// @throw std::invalid_argument if headers not enabled or header not found
  [[nodiscard]] std::string get_cell(size_type row, const std::string& col_header) const;

  /// @brief Get all column headers
  ///
  /// @return Vector of header names, or empty vector if headers not enabled
  [[nodiscard]] std::vector<std::string> get_headers() const;

  // ============================================================================
  // Iterator Support
  // ============================================================================

  /// @brief Get iterator to first data row
  /// @return Const iterator to first row
  [[nodiscard]] const_iterator begin() const noexcept;

  /// @brief Get iterator past last data row
  /// @return Const iterator past last row
  [[nodiscard]] const_iterator end() const noexcept;

  /// @brief Get const iterator to first data row
  /// @return Const iterator to first row
  [[nodiscard]] const_iterator cbegin() const noexcept;

  /// @brief Get const iterator past last data row
  /// @return Const iterator past last row
  [[nodiscard]] const_iterator cend() const noexcept;

  // ============================================================================
  // RFC 4180 Output
  // ============================================================================

  /// @brief Generate RFC 4180 compliant CSV output
  ///
  /// Produces normalized CSV output with:
  /// - Comma delimiter
  /// - CRLF line endings
  /// - Fields quoted only when necessary
  /// - Double quotes escaped as ""
  ///
  /// @return RFC 4180 compliant CSV string
  [[nodiscard]] std::string to_rfc_4180() const;

  /// @brief Write RFC 4180 compliant CSV to stream
  ///
  /// @param out Output stream to write to
  void write_rfc_4180(std::ostream& out) const;

  // ============================================================================
  // Configuration Access
  // ============================================================================

  /// @brief Get the parser configuration
  /// @return Reference to the configuration used for parsing
  [[nodiscard]] const csv_config& config() const noexcept;

private:
  // ============================================================================
  // Private Types
  // ============================================================================

  /// @brief Parser state machine states
  enum class parse_state
  {
    field_start,    ///< At the beginning of a field
    unquoted_field, ///< Inside an unquoted field
    quoted_field,   ///< Inside a quoted field
    after_quote     ///< Just saw a quote inside quoted field
  };

  // ============================================================================
  // Private Methods
  // ============================================================================

  /// @brief Main parsing function
  void parse(std::istream& stream);

  /// @brief Detect and skip UTF-8 BOM at stream start
  void detect_and_skip_bom(std::istream& stream);

  /// @brief Validate a string contains valid UTF-8
  void validate_utf8_string(const std::string& str, size_type line_number) const;

  /// @brief Finalize a row and add it to data
  void finalize_row(row_type& current_row, size_type line_number);

  /// @brief Build the header name to index map
  void build_header_index();

  /// @brief Apply whitespace trimming if configured
  [[nodiscard]] std::string apply_trimming(const std::string& field) const;

  /// @brief Check if a field needs quoting for RFC 4180 output
  [[nodiscard]] bool needs_quoting(const std::string& field) const;

  /// @brief Quote a field for RFC 4180 output
  [[nodiscard]] std::string quote_field(const std::string& field) const;

  /// @brief Write a single row in RFC 4180 format
  void write_row(std::ostream& out, const row_type& row) const;

  /// @brief Get column index for a header name
  [[nodiscard]] size_type column_index_for_header(const std::string& header_name) const;

  // ============================================================================
  // Member Variables
  // ============================================================================

  csv_config                                m_config;       ///< Parser configuration
  std::vector<std::string>                  m_headers;      ///< Column headers
  std::vector<row_type>                     m_data;         ///< Parsed data rows
  std::unordered_map<std::string, size_type> m_header_index; ///< Header to index map
  size_type                                 m_column_count{0}; ///< Number of columns
  std::string                               m_source_name;  ///< Source name for errors
};

} // namespace fb
