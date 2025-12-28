#include <fb/string_utils.h>

#include <gtest/gtest.h>

#include <limits>

namespace fb
{

// ============================================================================
// Trimming Tests
// ============================================================================

TEST(StringUtils, Trim_EmptyString_ReturnsEmpty)
{
  EXPECT_EQ(trim(""), "");
}

TEST(StringUtils, Trim_WhitespaceOnly_ReturnsEmpty)
{
  EXPECT_EQ(trim("   "), "");
  EXPECT_EQ(trim("\t\n\r"), "");
  EXPECT_EQ(trim(" \t \n "), "");
}

TEST(StringUtils, Trim_NoWhitespace_ReturnsSame)
{
  EXPECT_EQ(trim("hello"), "hello");
}

TEST(StringUtils, Trim_LeadingWhitespace_Removed)
{
  EXPECT_EQ(trim("   hello"), "hello");
  EXPECT_EQ(trim("\t\nhello"), "hello");
}

TEST(StringUtils, Trim_TrailingWhitespace_Removed)
{
  EXPECT_EQ(trim("hello   "), "hello");
  EXPECT_EQ(trim("hello\t\n"), "hello");
}

TEST(StringUtils, Trim_BothSides_BothRemoved)
{
  EXPECT_EQ(trim("   hello   "), "hello");
  EXPECT_EQ(trim("\t\nhello world\r\n"), "hello world");
}

TEST(StringUtils, TrimLeft_RemovesLeadingOnly)
{
  EXPECT_EQ(trim_left("   hello   "), "hello   ");
}

TEST(StringUtils, TrimRight_RemovesTrailingOnly)
{
  EXPECT_EQ(trim_right("   hello   "), "   hello");
}

TEST(StringUtils, TrimWithChars_RemovesSpecifiedChars)
{
  EXPECT_EQ(trim("xxhelloxx", "x"), "hello");
  EXPECT_EQ(trim("...hello...", "."), "hello");
  EXPECT_EQ(trim("abchelloabc", "abc"), "hello");
}

// ============================================================================
// Case Conversion Tests
// ============================================================================

TEST(StringUtils, ToUpper_Empty_ReturnsEmpty)
{
  EXPECT_EQ(to_upper(""), "");
}

TEST(StringUtils, ToUpper_AllLower_AllUpper)
{
  EXPECT_EQ(to_upper("hello"), "HELLO");
}

TEST(StringUtils, ToUpper_Mixed_AllUpper)
{
  EXPECT_EQ(to_upper("Hello World"), "HELLO WORLD");
}

TEST(StringUtils, ToUpper_NonAscii_PreservesNonAscii)
{
  EXPECT_EQ(to_upper("hello123"), "HELLO123");
}

TEST(StringUtils, ToLower_AllUpper_AllLower)
{
  EXPECT_EQ(to_lower("HELLO"), "hello");
}

TEST(StringUtils, ToLower_Mixed_AllLower)
{
  EXPECT_EQ(to_lower("Hello World"), "hello world");
}

TEST(StringUtils, Capitalize_Simple)
{
  EXPECT_EQ(capitalize("hello"), "Hello");
  EXPECT_EQ(capitalize("HELLO"), "Hello");
}

TEST(StringUtils, Capitalize_Empty)
{
  EXPECT_EQ(capitalize(""), "");
}

TEST(StringUtils, TitleCase_Simple)
{
  EXPECT_EQ(title_case("hello world"), "Hello World");
  EXPECT_EQ(title_case("HELLO WORLD"), "Hello World");
}

TEST(StringUtils, SwapCase_MixedCase)
{
  EXPECT_EQ(swap_case("Hello World"), "hELLO wORLD");
}

// ============================================================================
// Prefix/Suffix Tests
// ============================================================================

TEST(StringUtils, StartsWith_String_True)
{
  EXPECT_TRUE(starts_with("hello world", "hello"));
  EXPECT_TRUE(starts_with("hello", "hello"));
}

TEST(StringUtils, StartsWith_String_False)
{
  EXPECT_FALSE(starts_with("hello world", "world"));
  EXPECT_FALSE(starts_with("hello", "hello world"));
}

TEST(StringUtils, StartsWith_Empty)
{
  EXPECT_TRUE(starts_with("hello", ""));
  EXPECT_FALSE(starts_with("", "hello"));
}

TEST(StringUtils, StartsWith_Char_True)
{
  EXPECT_TRUE(starts_with("hello", 'h'));
}

TEST(StringUtils, StartsWith_Char_False)
{
  EXPECT_FALSE(starts_with("hello", 'e'));
  EXPECT_FALSE(starts_with("", 'h'));
}

TEST(StringUtils, EndsWith_String_True)
{
  EXPECT_TRUE(ends_with("hello world", "world"));
  EXPECT_TRUE(ends_with("hello", "hello"));
}

TEST(StringUtils, EndsWith_String_False)
{
  EXPECT_FALSE(ends_with("hello world", "hello"));
  EXPECT_FALSE(ends_with("world", "hello world"));
}

TEST(StringUtils, RemovePrefix_Present)
{
  EXPECT_EQ(remove_prefix("hello world", "hello "), "world");
}

TEST(StringUtils, RemovePrefix_NotPresent)
{
  EXPECT_EQ(remove_prefix("hello world", "xyz"), "hello world");
}

TEST(StringUtils, RemoveSuffix_Present)
{
  EXPECT_EQ(remove_suffix("hello world", " world"), "hello");
}

TEST(StringUtils, EnsurePrefix_NotPresent)
{
  EXPECT_EQ(ensure_prefix("world", "hello "), "hello world");
}

TEST(StringUtils, EnsurePrefix_AlreadyPresent)
{
  EXPECT_EQ(ensure_prefix("hello world", "hello"), "hello world");
}

TEST(StringUtils, EnsureSuffix_NotPresent)
{
  EXPECT_EQ(ensure_suffix("hello", ".txt"), "hello.txt");
}

TEST(StringUtils, EnsureSuffix_AlreadyPresent)
{
  EXPECT_EQ(ensure_suffix("hello.txt", ".txt"), "hello.txt");
}

// ============================================================================
// Searching Tests
// ============================================================================

TEST(StringUtils, Contains_Substring_True)
{
  EXPECT_TRUE(contains("hello world", "lo wo"));
}

TEST(StringUtils, Contains_Substring_False)
{
  EXPECT_FALSE(contains("hello world", "xyz"));
}

TEST(StringUtils, Contains_Empty)
{
  EXPECT_TRUE(contains("hello", ""));
  EXPECT_FALSE(contains("", "hello"));
}

TEST(StringUtils, Contains_Char_True)
{
  EXPECT_TRUE(contains("hello", 'e'));
}

TEST(StringUtils, Contains_Char_False)
{
  EXPECT_FALSE(contains("hello", 'x'));
}

TEST(StringUtils, Count_Substring)
{
  EXPECT_EQ(count("abababab", "ab"), 4u);
  EXPECT_EQ(count("hello", "l"), 2u);
  EXPECT_EQ(count("hello", "ll"), 1u);
  EXPECT_EQ(count("hello", "xyz"), 0u);
}

TEST(StringUtils, Count_Char)
{
  EXPECT_EQ(count("hello", 'l'), 2u);
  EXPECT_EQ(count("hello", 'x'), 0u);
}

TEST(StringUtils, FindAny_Found)
{
  EXPECT_EQ(find_any("hello world", "aeiou"), 1u);
}

TEST(StringUtils, FindAny_NotFound)
{
  EXPECT_EQ(find_any("xyz", "aeiou"), std::string::npos);
}

TEST(StringUtils, FindAnyLast_Found)
{
  EXPECT_EQ(find_any_last("hello world", "aeiou"), 7u);
}

// ============================================================================
// Replacement Tests
// ============================================================================

TEST(StringUtils, Replace_FirstOccurrence)
{
  EXPECT_EQ(replace("hello hello", "hello", "hi"), "hi hello");
}

TEST(StringUtils, ReplaceAll_AllOccurrences)
{
  EXPECT_EQ(replace_all("hello hello", "hello", "hi"), "hi hi");
}

TEST(StringUtils, ReplaceAll_Empty)
{
  EXPECT_EQ(replace_all("hello", "", "x"), "hello");
}

TEST(StringUtils, ReplaceAll_NotFound)
{
  EXPECT_EQ(replace_all("hello", "xyz", "abc"), "hello");
}

TEST(StringUtils, Replace_Char)
{
  EXPECT_EQ(replace("hello", 'l', 'L'), "heLlo");
}

TEST(StringUtils, ReplaceAll_Char)
{
  EXPECT_EQ(replace_all("hello", 'l', 'L'), "heLLo");
}

// ============================================================================
// Removal Tests
// ============================================================================

TEST(StringUtils, Remove_FirstOccurrence)
{
  EXPECT_EQ(remove("hello world hello", "hello"), " world hello");
}

TEST(StringUtils, RemoveAll_AllOccurrences)
{
  EXPECT_EQ(remove_all("hello world hello", "hello"), " world ");
}

TEST(StringUtils, Remove_Char)
{
  EXPECT_EQ(remove("hello", 'l'), "helo");
}

TEST(StringUtils, RemoveAll_Char)
{
  EXPECT_EQ(remove_all("hello", 'l'), "heo");
}

// ============================================================================
// Splitting Tests
// ============================================================================

TEST(StringUtils, Split_Char_Simple)
{
  auto parts = split("a,b,c", ',');
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
}

TEST(StringUtils, Split_Char_Empty)
{
  auto parts = split("", ',');
  ASSERT_EQ(parts.size(), 1u);
  EXPECT_EQ(parts[0], "");
}

TEST(StringUtils, Split_Char_NoDelimiter)
{
  auto parts = split("hello", ',');
  ASSERT_EQ(parts.size(), 1u);
  EXPECT_EQ(parts[0], "hello");
}

TEST(StringUtils, Split_Char_EmptyParts)
{
  auto parts = split("a,,b", ',');
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "");
  EXPECT_EQ(parts[2], "b");
}

TEST(StringUtils, Split_Char_SkipEmpty)
{
  auto parts = split("a,,b", ',', false);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
}

TEST(StringUtils, Split_String_Delimiter)
{
  auto parts = split("a::b::c", "::");
  ASSERT_EQ(parts.size(), 3u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b");
  EXPECT_EQ(parts[2], "c");
}

TEST(StringUtils, SplitLines_LF)
{
  auto lines = split_lines("line1\nline2\nline3");
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0], "line1");
  EXPECT_EQ(lines[1], "line2");
  EXPECT_EQ(lines[2], "line3");
}

TEST(StringUtils, SplitLines_CRLF)
{
  auto lines = split_lines("line1\r\nline2\r\nline3");
  ASSERT_EQ(lines.size(), 3u);
  EXPECT_EQ(lines[0], "line1");
  EXPECT_EQ(lines[1], "line2");
  EXPECT_EQ(lines[2], "line3");
}

TEST(StringUtils, SplitLines_Mixed)
{
  auto lines = split_lines("line1\nline2\r\nline3\rline4");
  ASSERT_EQ(lines.size(), 4u);
}

TEST(StringUtils, SplitWhitespace)
{
  auto parts = split_whitespace("  hello   world  ");
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], "hello");
  EXPECT_EQ(parts[1], "world");
}

TEST(StringUtils, SplitN_LimitParts)
{
  auto parts = split_n("a,b,c,d", ',', 2);
  ASSERT_EQ(parts.size(), 2u);
  EXPECT_EQ(parts[0], "a");
  EXPECT_EQ(parts[1], "b,c,d");
}

// ============================================================================
// Join Tests
// ============================================================================

TEST(StringUtils, Join_Empty)
{
  std::vector<std::string> parts;
  EXPECT_EQ(join(parts, ","), "");
}

TEST(StringUtils, Join_Single)
{
  std::vector<std::string> parts = {"hello"};
  EXPECT_EQ(join(parts, ","), "hello");
}

TEST(StringUtils, Join_Multiple)
{
  std::vector<std::string> parts = {"a", "b", "c"};
  EXPECT_EQ(join(parts, ","), "a,b,c");
  EXPECT_EQ(join(parts, "::"), "a::b::c");
}

TEST(StringUtils, Join_Char)
{
  std::vector<std::string> parts = {"a", "b", "c"};
  EXPECT_EQ(join(parts, ','), "a,b,c");
}

// ============================================================================
// Substring Tests
// ============================================================================

TEST(StringUtils, Left_Normal)
{
  EXPECT_EQ(left("hello", 2), "he");
  EXPECT_EQ(left("hello", 5), "hello");
  EXPECT_EQ(left("hello", 10), "hello");
}

TEST(StringUtils, Right_Normal)
{
  EXPECT_EQ(right("hello", 2), "lo");
  EXPECT_EQ(right("hello", 5), "hello");
  EXPECT_EQ(right("hello", 10), "hello");
}

TEST(StringUtils, Mid_Normal)
{
  EXPECT_EQ(mid("hello", 1, 3), "ell");
  EXPECT_EQ(mid("hello", 1), "ello");
  EXPECT_EQ(mid("hello", 10), "");
}

TEST(StringUtils, Slice_Positive)
{
  EXPECT_EQ(slice("hello", 1, 4), "ell");
  EXPECT_EQ(slice("hello", 0, 5), "hello");
}

TEST(StringUtils, Slice_Negative)
{
  EXPECT_EQ(slice("hello", -3), "llo");
  EXPECT_EQ(slice("hello", -3, -1), "ll");
  EXPECT_EQ(slice("hello", 0, -1), "hell");
}

TEST(StringUtils, Truncate_NoEllipsis)
{
  EXPECT_EQ(truncate("hello", 10), "hello");
  EXPECT_EQ(truncate("hello", 5), "hello");
}

TEST(StringUtils, Truncate_WithEllipsis)
{
  EXPECT_EQ(truncate("hello world", 8), "hello...");
  EXPECT_EQ(truncate("hello", 4), "h...");
}

// ============================================================================
// Padding Tests
// ============================================================================

TEST(StringUtils, PadLeft_Normal)
{
  EXPECT_EQ(pad_left("hello", 10), "     hello");
  EXPECT_EQ(pad_left("hello", 10, '0'), "00000hello");
  EXPECT_EQ(pad_left("hello", 3), "hello");
}

TEST(StringUtils, PadRight_Normal)
{
  EXPECT_EQ(pad_right("hello", 10), "hello     ");
  EXPECT_EQ(pad_right("hello", 10, '.'), "hello.....");
}

TEST(StringUtils, Center_Normal)
{
  EXPECT_EQ(center("hi", 6), "  hi  ");
  EXPECT_EQ(center("hi", 7), "  hi   ");
  EXPECT_EQ(center("hello", 3), "hello");
}

TEST(StringUtils, Repeat_Normal)
{
  EXPECT_EQ(repeat("ab", 3), "ababab");
  EXPECT_EQ(repeat("a", 5), "aaaaa");
  EXPECT_EQ(repeat("ab", 0), "");
}

// ============================================================================
// Validation Tests
// ============================================================================

TEST(StringUtils, IsEmpty)
{
  EXPECT_TRUE(is_empty(""));
  EXPECT_FALSE(is_empty(" "));
  EXPECT_FALSE(is_empty("hello"));
}

TEST(StringUtils, IsBlank)
{
  EXPECT_TRUE(is_blank(""));
  EXPECT_TRUE(is_blank("   "));
  EXPECT_TRUE(is_blank("\t\n\r"));
  EXPECT_FALSE(is_blank("hello"));
}

TEST(StringUtils, IsNumeric)
{
  EXPECT_TRUE(is_numeric("123"));
  EXPECT_TRUE(is_numeric("0"));
  EXPECT_FALSE(is_numeric(""));
  EXPECT_FALSE(is_numeric("-123"));
  EXPECT_FALSE(is_numeric("12.3"));
  EXPECT_FALSE(is_numeric("12a3"));
}

TEST(StringUtils, IsInteger)
{
  EXPECT_TRUE(is_integer("123"));
  EXPECT_TRUE(is_integer("-123"));
  EXPECT_TRUE(is_integer("+123"));
  EXPECT_FALSE(is_integer(""));
  EXPECT_FALSE(is_integer("12.3"));
  EXPECT_FALSE(is_integer("+"));
  EXPECT_FALSE(is_integer("-"));
}

TEST(StringUtils, IsFloat)
{
  EXPECT_TRUE(is_float("123"));
  EXPECT_TRUE(is_float("-123"));
  EXPECT_TRUE(is_float("12.3"));
  EXPECT_TRUE(is_float("-12.3"));
  EXPECT_TRUE(is_float("1e10"));
  EXPECT_TRUE(is_float("1.5e-10"));
  EXPECT_FALSE(is_float(""));
  EXPECT_FALSE(is_float("abc"));
  EXPECT_FALSE(is_float("1.2.3"));
}

TEST(StringUtils, IsAlpha)
{
  EXPECT_TRUE(is_alpha("Hello"));
  EXPECT_TRUE(is_alpha("abc"));
  EXPECT_FALSE(is_alpha(""));
  EXPECT_FALSE(is_alpha("Hello123"));
  EXPECT_FALSE(is_alpha("Hello World"));
}

TEST(StringUtils, IsAlphanumeric)
{
  EXPECT_TRUE(is_alphanumeric("Hello123"));
  EXPECT_TRUE(is_alphanumeric("abc"));
  EXPECT_TRUE(is_alphanumeric("123"));
  EXPECT_FALSE(is_alphanumeric(""));
  EXPECT_FALSE(is_alphanumeric("Hello World"));
}

TEST(StringUtils, IsHex)
{
  EXPECT_TRUE(is_hex("abc123"));
  EXPECT_TRUE(is_hex("0xabc123"));
  EXPECT_TRUE(is_hex("ABC123"));
  EXPECT_FALSE(is_hex(""));
  EXPECT_FALSE(is_hex("0x"));
  EXPECT_FALSE(is_hex("ghijk"));
}

TEST(StringUtils, IsBinary)
{
  EXPECT_TRUE(is_binary("101010"));
  EXPECT_TRUE(is_binary("0b101010"));
  EXPECT_FALSE(is_binary(""));
  EXPECT_FALSE(is_binary("0b"));
  EXPECT_FALSE(is_binary("12"));
}

TEST(StringUtils, IsAscii)
{
  EXPECT_TRUE(is_ascii("hello"));
  EXPECT_TRUE(is_ascii(""));
  EXPECT_TRUE(is_ascii("\t\n"));
}

TEST(StringUtils, IsPrintable)
{
  EXPECT_TRUE(is_printable("hello world"));
  EXPECT_TRUE(is_printable(""));
  EXPECT_FALSE(is_printable("hello\nworld"));
  EXPECT_FALSE(is_printable("hello\x01world"));
}

TEST(StringUtils, IsUpper)
{
  EXPECT_TRUE(is_upper("HELLO"));
  EXPECT_TRUE(is_upper("HELLO 123"));
  EXPECT_FALSE(is_upper("Hello"));
  EXPECT_FALSE(is_upper("123"));
}

TEST(StringUtils, IsLower)
{
  EXPECT_TRUE(is_lower("hello"));
  EXPECT_TRUE(is_lower("hello 123"));
  EXPECT_FALSE(is_lower("Hello"));
  EXPECT_FALSE(is_lower("123"));
}

TEST(StringUtils, IsIdentifier)
{
  EXPECT_TRUE(is_identifier("hello"));
  EXPECT_TRUE(is_identifier("_hello"));
  EXPECT_TRUE(is_identifier("hello123"));
  EXPECT_TRUE(is_identifier("_"));
  EXPECT_FALSE(is_identifier(""));
  EXPECT_FALSE(is_identifier("123hello"));
  EXPECT_FALSE(is_identifier("hello-world"));
}

// ============================================================================
// Number Parsing Tests
// ============================================================================

TEST(StringUtils, ToInt_Valid)
{
  EXPECT_EQ(to_int("123"), 123);
  EXPECT_EQ(to_int("-123"), -123);
  EXPECT_EQ(to_int("+123"), 123);
  EXPECT_EQ(to_int("  42  "), 42);
}

TEST(StringUtils, ToInt_Invalid)
{
  EXPECT_EQ(to_int(""), std::nullopt);
  EXPECT_EQ(to_int("abc"), std::nullopt);
  EXPECT_EQ(to_int("12.3"), std::nullopt);
  EXPECT_EQ(to_int("12a"), std::nullopt);
}

TEST(StringUtils, ToInt_WithBase)
{
  EXPECT_EQ(to_int("ff", 16), 255);
  EXPECT_EQ(to_int("0xff", 16), 255);
  EXPECT_EQ(to_int("1010", 2), 10);
  EXPECT_EQ(to_int("0b1010", 2), 10);
  EXPECT_EQ(to_int("77", 8), 63);
}

TEST(StringUtils, ToInt_BaseAutoDetect)
{
  // Base 0 means auto-detect from prefix
  EXPECT_EQ(to_int("0xff", 0), 255);
  EXPECT_EQ(to_int("0XFF", 0), 255);
  EXPECT_EQ(to_int("0b1010", 0), 10);
  EXPECT_EQ(to_int("0B1111", 0), 15);
  EXPECT_EQ(to_int("0o77", 0), 63);
  EXPECT_EQ(to_int("0O10", 0), 8);
  EXPECT_EQ(to_int("123", 0), 123);    // No prefix defaults to base 10
  EXPECT_EQ(to_int("042", 0), 42);     // Leading 0 without prefix is still base 10
}

TEST(StringUtils, ToLong_Valid)
{
  EXPECT_EQ(to_long("123456789"), 123456789L);
}

TEST(StringUtils, ToDouble_Valid)
{
  auto result = to_double("3.14159");
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(*result, 3.14159, 0.00001);
}

TEST(StringUtils, ToDouble_Invalid)
{
  EXPECT_EQ(to_double(""), std::nullopt);
  EXPECT_EQ(to_double("abc"), std::nullopt);
}

TEST(StringUtils, ToDouble_Scientific)
{
  auto result = to_double("1.5e10");
  ASSERT_TRUE(result.has_value());
  EXPECT_NEAR(*result, 1.5e10, 1e5);
}

// ============================================================================
// Number to String Tests
// ============================================================================

TEST(StringUtils, FromInt)
{
  EXPECT_EQ(from_int(123), "123");
  EXPECT_EQ(from_int(-123), "-123");
  EXPECT_EQ(from_int(0), "0");
}

TEST(StringUtils, FromInt_WithBase)
{
  EXPECT_EQ(from_int(255, 16), "ff");
  EXPECT_EQ(from_int(255, 16, true), "FF");
  EXPECT_EQ(from_int(10, 2), "1010");
}

TEST(StringUtils, FromDouble_Fixed)
{
  EXPECT_EQ(from_double_fixed(3.14159, 2), "3.14");
}

// ============================================================================
// Encoding Tests
// ============================================================================

TEST(StringUtils, ToHex_Empty)
{
  EXPECT_EQ(to_hex(""), "");
}

TEST(StringUtils, ToHex_Simple)
{
  EXPECT_EQ(to_hex("Hello"), "48656c6c6f");
  EXPECT_EQ(to_hex("Hello", true), "48656C6C6F");
}

TEST(StringUtils, FromHex_Valid)
{
  EXPECT_EQ(from_hex("48656c6c6f"), "Hello");
  EXPECT_EQ(from_hex("0x48656c6c6f"), "Hello");
}

TEST(StringUtils, FromHex_Invalid)
{
  EXPECT_EQ(from_hex("123"), std::nullopt);  // Odd length
  EXPECT_EQ(from_hex("gg"), std::nullopt);   // Invalid chars
}

TEST(StringUtils, ToBase64_Empty)
{
  EXPECT_EQ(to_base64(""), "");
}

TEST(StringUtils, ToBase64_Simple)
{
  EXPECT_EQ(to_base64("Hello"), "SGVsbG8=");
  EXPECT_EQ(to_base64("Hi"), "SGk=");
  EXPECT_EQ(to_base64("H"), "SA==");
}

TEST(StringUtils, FromBase64_Valid)
{
  EXPECT_EQ(from_base64("SGVsbG8="), "Hello");
  EXPECT_EQ(from_base64("SGk="), "Hi");
}

TEST(StringUtils, FromBase64_Invalid)
{
  EXPECT_EQ(from_base64("!!!"), std::nullopt);
}

TEST(StringUtils, UrlEncode)
{
  EXPECT_EQ(url_encode("Hello World"), "Hello%20World");
  EXPECT_EQ(url_encode("a=b&c=d"), "a%3Db%26c%3Dd");
}

TEST(StringUtils, UrlDecode_Valid)
{
  EXPECT_EQ(url_decode("Hello%20World"), "Hello World");
  EXPECT_EQ(url_decode("Hello+World"), "Hello World");
}

TEST(StringUtils, UrlDecode_Invalid)
{
  EXPECT_EQ(url_decode("%GG"), std::nullopt);
  EXPECT_EQ(url_decode("%2"), std::nullopt);
}

TEST(StringUtils, HtmlEscape)
{
  EXPECT_EQ(html_escape("<div>"), "&lt;div&gt;");
  EXPECT_EQ(html_escape("a & b"), "a &amp; b");
  EXPECT_EQ(html_escape("\"quoted\""), "&quot;quoted&quot;");
}

TEST(StringUtils, HtmlUnescape)
{
  EXPECT_EQ(html_unescape("&lt;div&gt;"), "<div>");
  EXPECT_EQ(html_unescape("a &amp; b"), "a & b");
}

TEST(StringUtils, JsonEscape)
{
  EXPECT_EQ(json_escape("hello\nworld"), "hello\\nworld");
  EXPECT_EQ(json_escape("hello\tworld"), "hello\\tworld");
  EXPECT_EQ(json_escape("\"quoted\""), "\\\"quoted\\\"");
}

TEST(StringUtils, JsonUnescape_Valid)
{
  EXPECT_EQ(json_unescape("hello\\nworld"), "hello\nworld");
  EXPECT_EQ(json_unescape("\\\"quoted\\\""), "\"quoted\"");
}

// ============================================================================
// Comparison Tests
// ============================================================================

TEST(StringUtils, CompareIgnoreCase)
{
  EXPECT_EQ(compare_ignore_case("hello", "HELLO"), 0);
  EXPECT_LT(compare_ignore_case("abc", "DEF"), 0);
  EXPECT_GT(compare_ignore_case("xyz", "ABC"), 0);
}

TEST(StringUtils, EqualsIgnoreCase)
{
  EXPECT_TRUE(equals_ignore_case("Hello", "hello"));
  EXPECT_TRUE(equals_ignore_case("HELLO", "hello"));
  EXPECT_FALSE(equals_ignore_case("hello", "world"));
}

TEST(StringUtils, NaturalCompare)
{
  EXPECT_LT(natural_compare("file2", "file10"), 0);
  EXPECT_GT(natural_compare("file10", "file2"), 0);
  EXPECT_EQ(natural_compare("file10", "file10"), 0);
  EXPECT_LT(natural_compare("a", "b"), 0);
}

// ============================================================================
// Miscellaneous Tests
// ============================================================================

TEST(StringUtils, Reverse)
{
  EXPECT_EQ(reverse("hello"), "olleh");
  EXPECT_EQ(reverse(""), "");
  EXPECT_EQ(reverse("a"), "a");
}

TEST(StringUtils, Squeeze)
{
  EXPECT_EQ(squeeze("aabbcc"), "abc");
  EXPECT_EQ(squeeze("hello"), "helo");
}

TEST(StringUtils, Squeeze_Char)
{
  EXPECT_EQ(squeeze("hello  world", ' '), "hello world");
}

TEST(StringUtils, NormalizeWhitespace)
{
  EXPECT_EQ(normalize_whitespace("  hello   world  "), "hello world");
  EXPECT_EQ(normalize_whitespace("\t\nhello\n\tworld\n"), "hello world");
}

TEST(StringUtils, WordWrap)
{
  std::string text = "hello world foo bar";
  std::string wrapped = word_wrap(text, 10);
  EXPECT_TRUE(wrapped.find('\n') != std::string::npos);
}

TEST(StringUtils, Indent)
{
  EXPECT_EQ(indent("hello\nworld", "  "), "  hello\n  world");
}

TEST(StringUtils, CommonPrefix)
{
  EXPECT_EQ(common_prefix("hello", "help"), "hel");
  EXPECT_EQ(common_prefix("hello", "world"), "");
}

TEST(StringUtils, CommonSuffix)
{
  EXPECT_EQ(common_suffix("hello", "jello"), "ello");
  EXPECT_EQ(common_suffix("hello", "world"), "");
}

TEST(StringUtils, LevenshteinDistance)
{
  EXPECT_EQ(levenshtein_distance("kitten", "sitting"), 3u);
  EXPECT_EQ(levenshtein_distance("hello", "hello"), 0u);
  EXPECT_EQ(levenshtein_distance("", "hello"), 5u);
}

} // namespace fb
