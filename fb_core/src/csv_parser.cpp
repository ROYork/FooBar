/// @file csv_parser.cpp
/// @brief Implementation of RFC 4180 compliant CSV parser

#include "fb/csv_parser.h"

#include <algorithm>
#include <cctype>

namespace fb
{

// ============================================================================
// Constructors
// ============================================================================

csv_parser::csv_parser(const std::string& filename)
    : csv_parser(filename, csv_config{})
{
}

csv_parser::csv_parser(const std::string& filename, const csv_config& config)
    : m_config(config), m_source_name(filename)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("csv_parser: cannot open file \"" + filename + "\"");
  }
  parse(file);
}

csv_parser::csv_parser(const std::filesystem::path& filepath)
    : csv_parser(filepath, csv_config{})
{
}

csv_parser::csv_parser(const std::filesystem::path& filepath, const csv_config& config)
    : m_config(config), m_source_name(filepath.string())
{
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("csv_parser: cannot open file \"" + filepath.string() + "\"");
  }
  parse(file);
}

csv_parser::csv_parser(std::istream& stream)
    : csv_parser(stream, csv_config{})
{
}

csv_parser::csv_parser(std::istream& stream, const csv_config& config)
    : m_config(config), m_source_name("<stream>")
{
  parse(stream);
}

// ============================================================================
// Dimension Methods
// ============================================================================

csv_parser::size_type csv_parser::row_count() const noexcept
{
  return m_data.size();
}

csv_parser::size_type csv_parser::column_count() const noexcept
{
  return m_column_count;
}

// ============================================================================
// Data Access Methods
// ============================================================================

csv_parser::row_type csv_parser::get_row(size_type row_index) const
{
  if (row_index >= m_data.size())
  {
    throw std::out_of_range("csv_parser::get_row: row index " + std::to_string(row_index) +
                            " out of range (row_count=" + std::to_string(m_data.size()) + ")");
  }
  return m_data[row_index];
}

std::vector<std::string> csv_parser::get_column(size_type col_index) const
{
  if (col_index >= m_column_count)
  {
    throw std::out_of_range("csv_parser::get_column: column index " + std::to_string(col_index) +
                            " out of range (column_count=" + std::to_string(m_column_count) + ")");
  }

  std::vector<std::string> result;
  result.reserve(m_data.size());

  for (const auto& row : m_data)
  {
    if (col_index < row.size())
    {
      result.push_back(row[col_index]);
    }
    else
    {
      // In non-strict mode, missing fields are empty strings
      result.emplace_back();
    }
  }

  return result;
}

std::vector<std::string> csv_parser::get_column(const std::string& header_name) const
{
  return get_column(column_index_for_header(header_name));
}

std::string csv_parser::get_cell(size_type row, size_type col) const
{
  if (row >= m_data.size())
  {
    throw std::out_of_range("csv_parser::get_cell: row index " + std::to_string(row) +
                            " out of range (row_count=" + std::to_string(m_data.size()) + ")");
  }
  if (col >= m_column_count)
  {
    throw std::out_of_range("csv_parser::get_cell: column index " + std::to_string(col) +
                            " out of range (column_count=" + std::to_string(m_column_count) + ")");
  }

  const auto& data_row = m_data[row];
  if (col < data_row.size())
  {
    return data_row[col];
  }

  // In non-strict mode, missing fields are empty strings
  return {};
}

std::string csv_parser::get_cell(size_type row, const std::string& col_header) const
{
  return get_cell(row, column_index_for_header(col_header));
}

std::vector<std::string> csv_parser::get_headers() const
{
  return m_headers;
}

// ============================================================================
// Iterator Support
// ============================================================================

csv_parser::const_iterator csv_parser::begin() const noexcept
{
  return m_data.begin();
}

csv_parser::const_iterator csv_parser::end() const noexcept
{
  return m_data.end();
}

csv_parser::const_iterator csv_parser::cbegin() const noexcept
{
  return m_data.cbegin();
}

csv_parser::const_iterator csv_parser::cend() const noexcept
{
  return m_data.cend();
}

// ============================================================================
// RFC 4180 Output
// ============================================================================

std::string csv_parser::to_rfc_4180() const
{
  std::ostringstream oss;
  write_rfc_4180(oss);
  return oss.str();
}

void csv_parser::write_rfc_4180(std::ostream& out) const
{
  // Write headers if enabled and present
  if (m_config.has_headers && !m_headers.empty())
  {
    write_row(out, m_headers);
  }

  // Write data rows
  for (const auto& row : m_data)
  {
    write_row(out, row);
  }
}

const csv_config& csv_parser::config() const noexcept
{
  return m_config;
}

// ============================================================================
// Private Methods
// ============================================================================

void csv_parser::parse(std::istream& stream)
{
  detect_and_skip_bom(stream);

  parse_state  state             = parse_state::field_start;
  size_type    line_number       = 1;
  size_type    field_start_line  = 1;
  row_type     current_row;
  std::string  current_field;

  char ch;
  while (stream.get(ch))
  {
    switch (state)
    {
    case parse_state::field_start:
      if (ch == '"')
      {
        state            = parse_state::quoted_field;
        field_start_line = line_number;
        current_field.clear();
      }
      else if (ch == m_config.delimiter)
      {
        // Empty field
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
      }
      else if (ch == '\r')
      {
        // Handle CRLF - peek for LF
        if (stream.peek() == '\n')
        {
          stream.get();
        }
        // Empty field at end of row
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
      }
      else if (ch == '\n')
      {
        // Empty field at end of row
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
      }
      else
      {
        current_field += ch;
        state = parse_state::unquoted_field;
      }
      break;

    case parse_state::unquoted_field:
      if (ch == '"')
      {
        if (m_config.strict_mode)
        {
          throw std::runtime_error("csv_parser: quote in unquoted field at line " +
                                   std::to_string(line_number) + " in " + m_source_name);
        }
        // Non-strict mode: treat quote as literal character
        current_field += ch;
      }
      else if (ch == m_config.delimiter)
      {
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
        state = parse_state::field_start;
      }
      else if (ch == '\r')
      {
        if (stream.peek() == '\n')
        {
          stream.get();
        }
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
        state = parse_state::field_start;
      }
      else if (ch == '\n')
      {
        current_row.push_back(apply_trimming(current_field));
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
        state = parse_state::field_start;
      }
      else
      {
        current_field += ch;
      }
      break;

    case parse_state::quoted_field:
      if (ch == '"')
      {
        state = parse_state::after_quote;
      }
      else
      {
        if (ch == '\n')
        {
          ++line_number;
        }
        current_field += ch;
      }
      break;

    case parse_state::after_quote:
      if (ch == '"')
      {
        // Escaped quote - add literal quote and continue quoted field
        current_field += '"';
        state = parse_state::quoted_field;
      }
      else if (ch == m_config.delimiter)
      {
        // End of quoted field, don't trim quoted fields
        current_row.push_back(current_field);
        current_field.clear();
        state = parse_state::field_start;
      }
      else if (ch == '\r')
      {
        if (stream.peek() == '\n')
        {
          stream.get();
        }
        current_row.push_back(current_field);
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
        state = parse_state::field_start;
      }
      else if (ch == '\n')
      {
        current_row.push_back(current_field);
        current_field.clear();
        finalize_row(current_row, line_number);
        ++line_number;
        state = parse_state::field_start;
      }
      else if (std::isspace(static_cast<unsigned char>(ch)))
      {
        if (m_config.strict_mode)
        {
          throw std::runtime_error("csv_parser: invalid character after closing quote at line " +
                                   std::to_string(line_number) + " in " + m_source_name);
        }
        // Non-strict mode: skip whitespace after closing quote
      }
      else
      {
        if (m_config.strict_mode)
        {
          throw std::runtime_error("csv_parser: invalid character after closing quote at line " +
                                   std::to_string(line_number) + " in " + m_source_name);
        }
        // Non-strict mode: treat as continuation of field data (unusual but tolerated)
        current_field += ch;
        state = parse_state::unquoted_field;
      }
      break;
    }
  }

  // Handle EOF
  if (state == parse_state::quoted_field)
  {
    throw std::runtime_error("csv_parser: unclosed quoted field starting at line " +
                             std::to_string(field_start_line) + " in " + m_source_name);
  }

  // Finalize last field/row if there's pending data
  if (state == parse_state::unquoted_field || state == parse_state::after_quote)
  {
    if (state == parse_state::after_quote)
    {
      current_row.push_back(current_field); // Don't trim quoted fields
    }
    else
    {
      current_row.push_back(apply_trimming(current_field));
    }
    finalize_row(current_row, line_number);
  }
  else if (state == parse_state::field_start && !current_row.empty())
  {
    // Row ended with delimiter, add empty final field
    current_row.push_back(apply_trimming(current_field));
    finalize_row(current_row, line_number);
  }

  // Build header index if headers are enabled
  if (m_config.has_headers)
  {
    build_header_index();
  }
}

void csv_parser::detect_and_skip_bom(std::istream& stream)
{
  // UTF-8 BOM: 0xEF 0xBB 0xBF
  unsigned char bom[3];
  auto bytes_read = stream.read(reinterpret_cast<char*>(bom), 3).gcount();

  if (bytes_read == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
  {
    // BOM detected and consumed - nothing more to do
    return;
  }

  // Not a BOM - seek back to start
  stream.clear();
  stream.seekg(0);
}

void csv_parser::validate_utf8_string(const std::string& str, size_type line_number) const
{
  for (size_t i = 0; i < str.size();)
  {
    auto c = static_cast<unsigned char>(str[i]);
    size_t expected_continuation = 0;

    if ((c & 0x80) == 0)
    {
      // ASCII: 0xxxxxxx
      ++i;
      continue;
    }
    else if ((c & 0xE0) == 0xC0)
    {
      // 2-byte: 110xxxxx
      expected_continuation = 1;
    }
    else if ((c & 0xF0) == 0xE0)
    {
      // 3-byte: 1110xxxx
      expected_continuation = 2;
    }
    else if ((c & 0xF8) == 0xF0)
    {
      // 4-byte: 11110xxx
      expected_continuation = 3;
    }
    else
    {
      throw std::runtime_error("csv_parser: invalid UTF-8 sequence at line " +
                               std::to_string(line_number) + " in " + m_source_name);
    }

    // Validate continuation bytes
    for (size_t j = 1; j <= expected_continuation; ++j)
    {
      if (i + j >= str.size() ||
          (static_cast<unsigned char>(str[i + j]) & 0xC0) != 0x80)
      {
        throw std::runtime_error("csv_parser: invalid UTF-8 continuation at line " +
                                 std::to_string(line_number) + " in " + m_source_name);
      }
    }
    i += 1 + expected_continuation;
  }
}

void csv_parser::finalize_row(row_type& current_row, size_type line_number)
{
  // Validate UTF-8 if configured
  if (m_config.validate_utf8)
  {
    for (const auto& field : current_row)
    {
      validate_utf8_string(field, line_number);
    }
  }

  // Check for empty row in strict mode
  if (m_config.strict_mode && current_row.empty())
  {
    throw std::runtime_error("csv_parser: empty row at line " +
                             std::to_string(line_number) + " in " + m_source_name);
  }

  // Handle completely empty row (just a newline)
  if (current_row.empty())
  {
    return; // Skip empty rows in non-strict mode
  }

  // Check for single empty field row
  if (current_row.size() == 1 && current_row[0].empty())
  {
    if (m_config.strict_mode)
    {
      throw std::runtime_error("csv_parser: empty row at line " +
                               std::to_string(line_number) + " in " + m_source_name);
    }
    // In non-strict mode, skip rows that are just empty
    current_row.clear();
    return;
  }

  bool is_header_row = m_config.has_headers && m_headers.empty() && m_data.empty();

  if (is_header_row)
  {
    m_headers      = std::move(current_row);
    m_column_count = m_headers.size();
  }
  else
  {
    // Check row consistency
    size_type expected_columns = m_column_count;
    if (expected_columns == 0)
    {
      // First data row determines column count when no headers
      expected_columns = current_row.size();
      m_column_count   = expected_columns;
    }

    if (m_config.strict_mode && current_row.size() != expected_columns)
    {
      throw std::runtime_error("csv_parser: inconsistent field count at line " +
                               std::to_string(line_number) + " (expected " +
                               std::to_string(expected_columns) + ", got " +
                               std::to_string(current_row.size()) + ") in " + m_source_name);
    }

    // In non-strict mode, update column count if this row has more fields
    if (!m_config.strict_mode && current_row.size() > m_column_count)
    {
      m_column_count = current_row.size();
    }

    m_data.push_back(std::move(current_row));
  }

  current_row.clear();
}

void csv_parser::build_header_index()
{
  for (size_type i = 0; i < m_headers.size(); ++i)
  {
    const auto& header = m_headers[i];
    auto [it, inserted] = m_header_index.emplace(header, i);
    if (!inserted)
    {
      throw std::invalid_argument("csv_parser: duplicate header \"" + header +
                                  "\" in " + m_source_name);
    }
  }
}

std::string csv_parser::apply_trimming(const std::string& field) const
{
  if (!m_config.trim_whitespace)
  {
    return field;
  }

  // Find first non-whitespace
  auto start = field.begin();
  while (start != field.end() && std::isspace(static_cast<unsigned char>(*start)))
  {
    ++start;
  }

  // Find last non-whitespace
  auto end_it = field.end();
  while (end_it != start && std::isspace(static_cast<unsigned char>(*(end_it - 1))))
  {
    --end_it;
  }

  return std::string(start, end_it);
}

bool csv_parser::needs_quoting(const std::string& field) const
{
  for (char ch : field)
  {
    if (ch == ',' || ch == '"' || ch == '\r' || ch == '\n')
    {
      return true;
    }
  }
  return false;
}

std::string csv_parser::quote_field(const std::string& field) const
{
  std::string result;
  result.reserve(field.size() + 2);
  result += '"';
  for (char ch : field)
  {
    if (ch == '"')
    {
      result += "\"\"";
    }
    else
    {
      result += ch;
    }
  }
  result += '"';
  return result;
}

void csv_parser::write_row(std::ostream& out, const row_type& row) const
{
  for (size_type i = 0; i < row.size(); ++i)
  {
    if (i > 0)
    {
      out << ',';
    }

    const auto& field = row[i];
    if (needs_quoting(field))
    {
      out << quote_field(field);
    }
    else
    {
      out << field;
    }
  }
  out << "\r\n"; // RFC 4180 mandates CRLF
}

csv_parser::size_type csv_parser::column_index_for_header(const std::string& header_name) const
{
  if (!m_config.has_headers)
  {
    throw std::invalid_argument("csv_parser: headers not enabled");
  }

  auto it = m_header_index.find(header_name);
  if (it == m_header_index.end())
  {
    throw std::invalid_argument("csv_parser: header \"" + header_name + "\" not found");
  }

  return it->second;
}

} // namespace fb
