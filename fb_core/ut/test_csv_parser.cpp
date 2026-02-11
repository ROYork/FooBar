/// @file test_csv_parser.cpp
/// @brief Comprehensive unit tests for csv_parser class

#include <gtest/gtest.h>

#include <fb/csv_parser.h>

#include <sstream>
#include <fstream>
#include <filesystem>

using namespace fb;

// ============================================================================
// Helper Functions
// ============================================================================

namespace
{

/// @brief Create a parser from a string
csv_parser make_parser(const std::string& csv_data, const csv_config& config = {})
{
  std::istringstream stream(csv_data);
  return csv_parser(stream, config);
}

} // namespace

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(CSVParserTest, SimpleCSV)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.column_count(), 2u);
}

TEST(CSVParserTest, RowCount)
{
  auto parser = make_parser("A,B\n1,2\n3,4\n5,6\n");

  EXPECT_EQ(parser.row_count(), 3u);
}

TEST(CSVParserTest, ColumnCount)
{
  auto parser = make_parser("A,B,C,D\n1,2,3,4\n");

  EXPECT_EQ(parser.column_count(), 4u);
}

TEST(CSVParserTest, GetRow)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  auto row0 = parser.get_row(0);
  ASSERT_EQ(row0.size(), 2u);
  EXPECT_EQ(row0[0], "Alice");
  EXPECT_EQ(row0[1], "30");

  auto row1 = parser.get_row(1);
  ASSERT_EQ(row1.size(), 2u);
  EXPECT_EQ(row1[0], "Bob");
  EXPECT_EQ(row1[1], "25");
}

TEST(CSVParserTest, GetColumnByIndex)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  auto col0 = parser.get_column(0);
  ASSERT_EQ(col0.size(), 2u);
  EXPECT_EQ(col0[0], "Alice");
  EXPECT_EQ(col0[1], "Bob");

  auto col1 = parser.get_column(1);
  ASSERT_EQ(col1.size(), 2u);
  EXPECT_EQ(col1[0], "30");
  EXPECT_EQ(col1[1], "25");
}

TEST(CSVParserTest, GetColumnByName)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  auto names = parser.get_column("Name");
  ASSERT_EQ(names.size(), 2u);
  EXPECT_EQ(names[0], "Alice");
  EXPECT_EQ(names[1], "Bob");

  auto ages = parser.get_column("Age");
  ASSERT_EQ(ages.size(), 2u);
  EXPECT_EQ(ages[0], "30");
  EXPECT_EQ(ages[1], "25");
}

TEST(CSVParserTest, GetCellByIndex)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  EXPECT_EQ(parser.get_cell(0, 0), "Alice");
  EXPECT_EQ(parser.get_cell(0, 1), "30");
  EXPECT_EQ(parser.get_cell(1, 0), "Bob");
  EXPECT_EQ(parser.get_cell(1, 1), "25");
}

TEST(CSVParserTest, GetCellByName)
{
  auto parser = make_parser("Name,Age\nAlice,30\nBob,25\n");

  EXPECT_EQ(parser.get_cell(0, "Name"), "Alice");
  EXPECT_EQ(parser.get_cell(0, "Age"), "30");
  EXPECT_EQ(parser.get_cell(1, "Name"), "Bob");
  EXPECT_EQ(parser.get_cell(1, "Age"), "25");
}

TEST(CSVParserTest, GetHeaders)
{
  auto parser = make_parser("Name,Age,City\nAlice,30,NYC\n");

  auto headers = parser.get_headers();
  ASSERT_EQ(headers.size(), 3u);
  EXPECT_EQ(headers[0], "Name");
  EXPECT_EQ(headers[1], "Age");
  EXPECT_EQ(headers[2], "City");
}

// ============================================================================
// RFC 4180 Compliance Tests
// ============================================================================

TEST(CSVParserTest, QuotedFieldsWithCommas)
{
  auto parser = make_parser("Name,Location\n\"Smith, John\",NYC\n", {});

  EXPECT_EQ(parser.get_cell(0, "Name"), "Smith, John");
  EXPECT_EQ(parser.get_cell(0, "Location"), "NYC");
}

TEST(CSVParserTest, QuotedFieldsWithQuotes)
{
  auto parser = make_parser("Name,Quote\nAlice,\"He said \"\"Hello\"\"\"\n", {});

  EXPECT_EQ(parser.get_cell(0, "Quote"), "He said \"Hello\"");
}

TEST(CSVParserTest, MultiLineFields)
{
  auto parser = make_parser("Name,Bio\nAlice,\"Line1\nLine2\nLine3\"\n", {});

  EXPECT_EQ(parser.get_cell(0, "Bio"), "Line1\nLine2\nLine3");
}

TEST(CSVParserTest, CRLFLineEndings)
{
  auto parser = make_parser("A,B\r\n1,2\r\n3,4\r\n", {});

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.get_cell(0, 0), "1");
  EXPECT_EQ(parser.get_cell(1, 0), "3");
}

TEST(CSVParserTest, LFLineEndings)
{
  auto parser = make_parser("A,B\n1,2\n3,4\n", {});

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.get_cell(0, 0), "1");
  EXPECT_EQ(parser.get_cell(1, 0), "3");
}

TEST(CSVParserTest, MixedLineEndings)
{
  auto parser = make_parser("A,B\r\n1,2\n3,4\r\n", {});

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.get_cell(0, 0), "1");
  EXPECT_EQ(parser.get_cell(1, 0), "3");
}

TEST(CSVParserTest, LineEndingPreservationInQuoted)
{
  // Test that CRLF inside quoted field is preserved
  auto parser = make_parser("A,B\n\"Line1\r\nLine2\",C\n", {});

  EXPECT_EQ(parser.get_cell(0, 0), "Line1\r\nLine2");
}

TEST(CSVParserTest, EscapedDoubleQuotes)
{
  auto parser = make_parser("A\n\"a\"\"b\"\"c\"\n", {});

  EXPECT_EQ(parser.get_cell(0, 0), "a\"b\"c");
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST(CSVParserTest, HasHeadersTrue)
{
  csv_config config;
  config.has_headers = true;

  auto parser = make_parser("Name,Age\nAlice,30\n", config);

  EXPECT_EQ(parser.row_count(), 1u);
  EXPECT_EQ(parser.get_headers().size(), 2u);
}

TEST(CSVParserTest, HasHeadersFalse)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("Name,Age\nAlice,30\n", config);

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_TRUE(parser.get_headers().empty());
}

TEST(CSVParserTest, TrimWhitespaceTrue)
{
  csv_config config;
  config.has_headers     = false;
  config.trim_whitespace = true;

  auto parser = make_parser("  hello  ,  world  \n", config);

  auto row = parser.get_row(0);
  EXPECT_EQ(row[0], "hello");
  EXPECT_EQ(row[1], "world");
}

TEST(CSVParserTest, TrimWhitespaceFalse)
{
  csv_config config;
  config.has_headers     = false;
  config.trim_whitespace = false;

  auto parser = make_parser("  hello  ,  world  \n", config);

  auto row = parser.get_row(0);
  EXPECT_EQ(row[0], "  hello  ");
  EXPECT_EQ(row[1], "  world  ");
}

TEST(CSVParserTest, TrimWhitespaceDoesNotAffectQuoted)
{
  csv_config config;
  config.has_headers     = false;
  config.trim_whitespace = true;

  auto parser = make_parser("\"  hello  \",  world  \n", config);

  auto row = parser.get_row(0);
  EXPECT_EQ(row[0], "  hello  "); // Quoted - not trimmed
  EXPECT_EQ(row[1], "world");     // Unquoted - trimmed
}

TEST(CSVParserTest, CustomDelimiter)
{
  csv_config config;
  config.has_headers = false;
  config.delimiter   = ';';

  auto parser = make_parser("a;b;c\n1;2;3\n", config);

  EXPECT_EQ(parser.column_count(), 3u);
  EXPECT_EQ(parser.get_cell(0, 0), "a");
  EXPECT_EQ(parser.get_cell(1, 2), "3");
}

TEST(CSVParserTest, StrictModeTrue_ConsistentFields)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  auto parser = make_parser("A,B\n1,2\n3,4\n", config);

  EXPECT_EQ(parser.row_count(), 2u);
}

TEST(CSVParserTest, StrictModeFalse_VariableFields)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = false;

  auto parser = make_parser("A,B,C\n1,2\n3,4,5,6\n", config);

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.column_count(), 4u); // Max fields across all rows

  // Missing field returns empty string
  EXPECT_EQ(parser.get_cell(0, 2), "");
}

TEST(CSVParserTest, NonStrictMode_QuotesInUnquotedField)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = false;

  auto parser = make_parser("Name,Doc\nElement_ID,Unique Identifier of the \"partitioned\" Domain element\n", config);

  EXPECT_EQ(parser.row_count(), 1u);
  EXPECT_EQ(parser.get_cell(0, "Doc"), "Unique Identifier of the \"partitioned\" Domain element");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(CSVParserTest, RowIndexOutOfRange)
{
  auto parser = make_parser("A\n1\n", {});

  EXPECT_THROW(parser.get_row(5), std::out_of_range);
}

TEST(CSVParserTest, ColumnIndexOutOfRange)
{
  auto parser = make_parser("A,B\n1,2\n", {});

  EXPECT_THROW(parser.get_column(10), std::out_of_range);
}

TEST(CSVParserTest, CellRowIndexOutOfRange)
{
  auto parser = make_parser("A\n1\n", {});

  EXPECT_THROW(parser.get_cell(10, 0), std::out_of_range);
}

TEST(CSVParserTest, CellColumnIndexOutOfRange)
{
  auto parser = make_parser("A\n1\n", {});

  EXPECT_THROW(parser.get_cell(0, 10), std::out_of_range);
}

TEST(CSVParserTest, InvalidHeaderName)
{
  auto parser = make_parser("Name,Age\nAlice,30\n", {});

  EXPECT_THROW(parser.get_column("NonExistent"), std::invalid_argument);
}

TEST(CSVParserTest, HeaderAccessWhenDisabled)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("A,B\n1,2\n", config);

  EXPECT_THROW(parser.get_column("A"), std::invalid_argument);
}

TEST(CSVParserTest, UnclosedQuote)
{
  csv_config config;
  config.has_headers = false;

  EXPECT_THROW(make_parser("\"unclosed\n", config), std::runtime_error);
}

TEST(CSVParserTest, StrictMode_QuoteInUnquotedField)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("hello\"world\n", config), std::runtime_error);
}

TEST(CSVParserTest, StrictMode_InconsistentFieldCount)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("a,b,c\n1,2\n", config), std::runtime_error);
}

TEST(CSVParserTest, DuplicateHeaders)
{
  csv_config config;
  config.has_headers = true;

  EXPECT_THROW(make_parser("Name,Name\n1,2\n", config), std::invalid_argument);
}

// ============================================================================
// UTF-8 BOM Tests
// ============================================================================

TEST(CSVParserTest, UTF8BOMDetection)
{
  // UTF-8 BOM: 0xEF 0xBB 0xBF
  std::string csv_with_bom = "\xEF\xBB\xBFName,Age\nAlice,30\n";

  auto parser = make_parser(csv_with_bom, {});

  auto headers = parser.get_headers();
  ASSERT_EQ(headers.size(), 2u);
  EXPECT_EQ(headers[0], "Name"); // BOM should be removed
  EXPECT_EQ(headers[1], "Age");
}

TEST(CSVParserTest, NoBOMHandling)
{
  auto parser = make_parser("Name,Age\nAlice,30\n", {});

  auto headers = parser.get_headers();
  EXPECT_EQ(headers[0], "Name");
}

// ============================================================================
// UTF-8 Validation Tests
// ============================================================================

TEST(CSVParserTest, ValidUTF8)
{
  csv_config config;
  config.has_headers   = false;
  config.validate_utf8 = true;

  // Valid UTF-8: "Hello, \xe4\xb8\x96\xe7\x95\x8c" = "Hello, 世界"
  auto parser = make_parser("Hello,\xe4\xb8\x96\xe7\x95\x8c\n", config);

  EXPECT_EQ(parser.row_count(), 1u);
}

TEST(CSVParserTest, InvalidUTF8_Strict)
{
  csv_config config;
  config.has_headers   = false;
  config.validate_utf8 = true;

  // Invalid UTF-8: lone continuation byte
  EXPECT_THROW(make_parser("Hello,\x80world\n", config), std::runtime_error);
}

TEST(CSVParserTest, InvalidUTF8_NoValidation)
{
  csv_config config;
  config.has_headers   = false;
  config.validate_utf8 = false;

  // Invalid UTF-8 passes through when validation is disabled
  auto parser = make_parser("Hello,\x80world\n", config);
  EXPECT_EQ(parser.row_count(), 1u);
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(CSVParserTest, RangeBasedForLoop)
{
  auto parser = make_parser("A,B\n1,2\n3,4\n5,6\n", {});

  int count = 0;
  for (const auto& row : parser)
  {
    EXPECT_EQ(row.size(), 2u);
    ++count;
  }
  EXPECT_EQ(count, 3);
}

TEST(CSVParserTest, BeginEnd)
{
  auto parser = make_parser("A\n1\n2\n3\n", {});

  auto it = parser.begin();
  EXPECT_EQ((*it)[0], "1");
  ++it;
  EXPECT_EQ((*it)[0], "2");
  ++it;
  EXPECT_EQ((*it)[0], "3");
  ++it;
  EXPECT_EQ(it, parser.end());
}

TEST(CSVParserTest, CBeginCEnd)
{
  auto parser = make_parser("A\n1\n", {});

  auto it = parser.cbegin();
  EXPECT_NE(it, parser.cend());
  ++it;
  EXPECT_EQ(it, parser.cend());
}

TEST(CSVParserTest, EmptyIterator)
{
  auto parser = make_parser("A,B\n", {});

  EXPECT_EQ(parser.begin(), parser.end());
}

// ============================================================================
// RFC 4180 Output Tests
// ============================================================================

TEST(CSVParserTest, ToRfc4180Simple)
{
  auto parser = make_parser("A,B\n1,2\n3,4\n", {});

  std::string output = parser.to_rfc_4180();

  EXPECT_EQ(output, "A,B\r\n1,2\r\n3,4\r\n");
}

TEST(CSVParserTest, ToRfc4180WithQuoting)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("hello,world\nfoo,\"bar,baz\"\n", config);

  std::string output = parser.to_rfc_4180();

  // Fields with commas should be quoted
  EXPECT_EQ(output, "hello,world\r\nfoo,\"bar,baz\"\r\n");
}

TEST(CSVParserTest, ToRfc4180EscapesQuotes)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("a,\"b\"\"c\"\n", config);

  std::string output = parser.to_rfc_4180();

  EXPECT_EQ(output, "a,\"b\"\"c\"\r\n");
}

TEST(CSVParserTest, WriteRfc4180ToStream)
{
  auto parser = make_parser("A\n1\n2\n", {});

  std::ostringstream oss;
  parser.write_rfc_4180(oss);

  EXPECT_EQ(oss.str(), "A\r\n1\r\n2\r\n");
}

TEST(CSVParserTest, RoundTripPreservation)
{
  std::string original = "Name,Age\r\nAlice,30\r\nBob,25\r\n";

  auto parser1 = make_parser(original, {});
  std::string output = parser1.to_rfc_4180();

  auto parser2 = make_parser(output, {});

  EXPECT_EQ(parser1.row_count(), parser2.row_count());
  EXPECT_EQ(parser1.column_count(), parser2.column_count());

  for (size_t i = 0; i < parser1.row_count(); ++i)
  {
    EXPECT_EQ(parser1.get_row(i), parser2.get_row(i));
  }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(CSVParserTest, SingleRowSingleColumn)
{
  auto parser = make_parser("A\n1\n", {});

  EXPECT_EQ(parser.row_count(), 1u);
  EXPECT_EQ(parser.column_count(), 1u);
  EXPECT_EQ(parser.get_cell(0, 0), "1");
}

TEST(CSVParserTest, SingleRow)
{
  auto parser = make_parser("A,B,C\n1,2,3\n", {});

  EXPECT_EQ(parser.row_count(), 1u);
}

TEST(CSVParserTest, SingleColumn)
{
  auto parser = make_parser("A\n1\n2\n3\n", {});

  EXPECT_EQ(parser.column_count(), 1u);
}

TEST(CSVParserTest, TrailingNewline)
{
  auto parser = make_parser("A\n1\n2\n", {});

  EXPECT_EQ(parser.row_count(), 2u);
}

TEST(CSVParserTest, NoTrailingNewline)
{
  auto parser = make_parser("A\n1\n2", {});

  EXPECT_EQ(parser.row_count(), 2u);
}

TEST(CSVParserTest, EmptyFields)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("a,,c\n,b,\n", config);

  EXPECT_EQ(parser.get_cell(0, 0), "a");
  EXPECT_EQ(parser.get_cell(0, 1), "");
  EXPECT_EQ(parser.get_cell(0, 2), "c");
  EXPECT_EQ(parser.get_cell(1, 0), "");
  EXPECT_EQ(parser.get_cell(1, 1), "b");
  EXPECT_EQ(parser.get_cell(1, 2), "");
}

TEST(CSVParserTest, QuotedEmptyField)
{
  csv_config config;
  config.has_headers = false;

  auto parser = make_parser("a,\"\",c\n", config);

  EXPECT_EQ(parser.get_cell(0, 1), "");
}

TEST(CSVParserTest, ConfigAccess)
{
  csv_config config;
  config.has_headers     = false;
  config.trim_whitespace = true;
  config.delimiter       = ';';

  auto parser = make_parser("a;b;c\n", config);

  const auto& cfg = parser.config();
  EXPECT_FALSE(cfg.has_headers);
  EXPECT_TRUE(cfg.trim_whitespace);
  EXPECT_EQ(cfg.delimiter, ';');
}

// ============================================================================
// Normative Test Vectors (from requirements document)
// ============================================================================

TEST(CSVParserTest, TV001_EmptyFile)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  auto parser = make_parser("", config);

  EXPECT_EQ(parser.row_count(), 0u);
  EXPECT_EQ(parser.column_count(), 0u);
  EXPECT_THROW(parser.get_row(0), std::out_of_range);
}

TEST(CSVParserTest, TV002_HeaderOnly)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  auto parser = make_parser("Name,Age\n", config);

  EXPECT_EQ(parser.row_count(), 0u);
  EXPECT_EQ(parser.column_count(), 2u);

  auto headers = parser.get_headers();
  ASSERT_EQ(headers.size(), 2u);
  EXPECT_EQ(headers[0], "Name");
  EXPECT_EQ(headers[1], "Age");
}

TEST(CSVParserTest, TV003_EmptyFields)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  auto parser = make_parser("field1,,field3\n", config);

  auto row = parser.get_row(0);
  ASSERT_EQ(row.size(), 3u);
  EXPECT_EQ(row[0], "field1");
  EXPECT_EQ(row[1], "");
  EXPECT_EQ(row[2], "field3");
}

TEST(CSVParserTest, TV004_QuotedFieldWithCommasAndQuotes)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  auto parser = make_parser("\"Smith, John\",\"He said \"\"Hello\"\"\"\n", config);

  auto row = parser.get_row(0);
  ASSERT_EQ(row.size(), 2u);
  EXPECT_EQ(row[0], "Smith, John");
  EXPECT_EQ(row[1], "He said \"Hello\"");
}

TEST(CSVParserTest, TV005_MultiLineFieldWithMixedEndings)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  // CRLF between records; LF inside quoted field
  auto parser = make_parser("\"A\",\"B\"\r\n\"Line1\nLine2\",\"C\"\r\n", config);

  EXPECT_EQ(parser.row_count(), 1u);
  EXPECT_EQ(parser.column_count(), 2u);
  EXPECT_EQ(parser.get_cell(0, 0), "Line1\nLine2"); // LF preserved
}

TEST(CSVParserTest, TV006_InconsistentRowLengthsStrict)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("a,b,c\n1,2\n", config), std::runtime_error);
}

TEST(CSVParserTest, TV007_InconsistentRowLengthsNonStrict)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = false;

  auto parser = make_parser("a,b,c\n1,2\n", config);

  EXPECT_EQ(parser.row_count(), 2u);
  EXPECT_EQ(parser.get_cell(1, 2), ""); // Missing field is empty string
}

TEST(CSVParserTest, TV008_RandomQuotesInUnquotedFieldNonStrict)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = false;

  auto parser = make_parser("Name,Doc\nElement_ID,Unique Identifier of the \"partitioned\" Domain element\n", config);

  EXPECT_EQ(parser.row_count(), 1u);
  // Quotes are literal characters
  EXPECT_EQ(parser.get_cell(0, "Doc"), "Unique Identifier of the \"partitioned\" Domain element");
}

TEST(CSVParserTest, TV009_RandomQuotesInUnquotedFieldStrict)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("Name,Doc\nElement_ID,Unique Identifier of the \"partitioned\" Domain element\n", config),
               std::runtime_error);
}

TEST(CSVParserTest, TV010_DuplicateHeaders)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("Name,Name\n1,2\n", config), std::invalid_argument);
}

TEST(CSVParserTest, TV011_UTF8BOMAtStart)
{
  csv_config config;
  config.has_headers = true;
  config.strict_mode = true;

  // UTF-8 BOM + "Name,Age\nAlice,30\n"
  std::string csv = "\xEF\xBB\xBFName,Age\nAlice,30\n";

  auto parser = make_parser(csv, config);

  auto headers = parser.get_headers();
  EXPECT_EQ(headers[0], "Name"); // BOM removed from first field
}

TEST(CSVParserTest, TV012_WhitespaceTrimming)
{
  csv_config config;
  config.has_headers     = false;
  config.strict_mode     = true;
  config.trim_whitespace = true;

  auto parser = make_parser("  a ,  b  \n", config);

  auto row = parser.get_row(0);
  ASSERT_EQ(row.size(), 2u);
  EXPECT_EQ(row[0], "a");
  EXPECT_EQ(row[1], "b");
}

TEST(CSVParserTest, TV013_UnclosedQuotedField)
{
  csv_config config;
  config.has_headers = false;
  config.strict_mode = true;

  EXPECT_THROW(make_parser("\"a,b\n", config), std::runtime_error);
}

// ============================================================================
// File-Based Tests
// ============================================================================

TEST(CSVParserTest, ConstructFromFilePath)
{
  // Create a temporary file
  std::filesystem::path temp_path = std::filesystem::temp_directory_path() / "test_csv_parser.csv";

  {
    std::ofstream file(temp_path);
    file << "Name,Age\nAlice,30\nBob,25\n";
  }

  // Test construction from string path
  csv_parser parser1(temp_path.string());
  EXPECT_EQ(parser1.row_count(), 2u);

  // Test construction from filesystem::path
  csv_parser parser2(temp_path);
  EXPECT_EQ(parser2.row_count(), 2u);

  // Cleanup
  std::filesystem::remove(temp_path);
}

TEST(CSVParserTest, FileNotFound)
{
  EXPECT_THROW(csv_parser(std::string("/nonexistent/path/file.csv")), std::runtime_error);
}
