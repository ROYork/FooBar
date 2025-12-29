#include <fb/format.h>

#include <gtest/gtest.h>

#include <cmath>
#include <limits>

namespace fb
{

// ============================================================================
// Basic Formatting Tests
// ============================================================================

TEST(Format, EmptyFormat)
{
  EXPECT_EQ(format(""), "");
}

TEST(Format, NoPlaceholders)
{
  EXPECT_EQ(format("Hello, World!"), "Hello, World!");
}

TEST(Format, EscapedBraces)
{
  EXPECT_EQ(format("{{}}"), "{}");
  EXPECT_EQ(format("{{hello}}"), "{hello}");
}

// ============================================================================
// String Formatting Tests
// ============================================================================

TEST(Format, StringSimple)
{
  EXPECT_EQ(format("Hello, {}!", "World"), "Hello, World!");
}

TEST(Format, StringMultiple)
{
  EXPECT_EQ(format("{} {} {}", "a", "b", "c"), "a b c");
}

TEST(Format, StringPositional)
{
  EXPECT_EQ(format("{1} {0}", "World", "Hello"), "Hello World");
  EXPECT_EQ(format("{0} {0}", "hello"), "hello hello");
}

TEST(Format, StringAlignment)
{
  EXPECT_EQ(format("{:<10}", "hi"), "hi        ");
  EXPECT_EQ(format("{:>10}", "hi"), "        hi");
  EXPECT_EQ(format("{:^10}", "hi"), "    hi    ");
}

TEST(Format, StringFill)
{
  EXPECT_EQ(format("{:*<10}", "hi"), "hi********");
  EXPECT_EQ(format("{:*>10}", "hi"), "********hi");
  EXPECT_EQ(format("{:*^10}", "hi"), "****hi****");
}

TEST(Format, StringPrecision)
{
  EXPECT_EQ(format("{:.3}", "hello"), "hel");
  EXPECT_EQ(format("{:.10}", "hello"), "hello");
}

TEST(Format, StringView)
{
  std::string_view sv = "test";
  EXPECT_EQ(format("{}", sv), "test");
}

TEST(Format, ConstCharPtr)
{
  const char* str = "test";
  EXPECT_EQ(format("{}", str), "test");
}

TEST(Format, NullString)
{
  const char* null_str = nullptr;
  EXPECT_EQ(format("{}", null_str), "(null)");
}

// ============================================================================
// Integer Formatting Tests
// ============================================================================

TEST(Format, IntSimple)
{
  EXPECT_EQ(format("{}", 42), "42");
  EXPECT_EQ(format("{}", -42), "-42");
  EXPECT_EQ(format("{}", 0), "0");
}

TEST(Format, IntWidth)
{
  EXPECT_EQ(format("{:5}", 42), "   42");
  EXPECT_EQ(format("{:5}", -42), "  -42");
}

TEST(Format, IntZeroPad)
{
  EXPECT_EQ(format("{:05}", 42), "00042");
  EXPECT_EQ(format("{:05}", -42), "-0042");
}

TEST(Format, IntSign)
{
  EXPECT_EQ(format("{:+}", 42), "+42");
  EXPECT_EQ(format("{:+}", -42), "-42");
  EXPECT_EQ(format("{: }", 42), " 42");
  EXPECT_EQ(format("{: }", -42), "-42");
}

TEST(Format, IntHex)
{
  EXPECT_EQ(format("{:x}", 255), "ff");
  EXPECT_EQ(format("{:X}", 255), "FF");
  EXPECT_EQ(format("{:#x}", 255), "0xff");
  EXPECT_EQ(format("{:#X}", 255), "0XFF");
}

TEST(Format, IntOctal)
{
  EXPECT_EQ(format("{:o}", 8), "10");
  EXPECT_EQ(format("{:#o}", 8), "010");
}

TEST(Format, IntBinary)
{
  EXPECT_EQ(format("{:b}", 10), "1010");
  EXPECT_EQ(format("{:#b}", 10), "0b1010");
  EXPECT_EQ(format("{:#B}", 10), "0B1010");
}

TEST(Format, IntAlignment)
{
  EXPECT_EQ(format("{:<5}", 42), "42   ");
  EXPECT_EQ(format("{:>5}", 42), "   42");
  EXPECT_EQ(format("{:^5}", 42), " 42  ");
}

TEST(Format, IntTypes)
{
  EXPECT_EQ(format("{}", static_cast<short>(123)), "123");
  EXPECT_EQ(format("{}", static_cast<long>(123456)), "123456");
  EXPECT_EQ(format("{}", static_cast<long long>(123456789)), "123456789");
  EXPECT_EQ(format("{}", static_cast<unsigned>(42)), "42");
}

// ============================================================================
// Floating-Point Formatting Tests
// ============================================================================

TEST(Format, FloatSimple)
{
  EXPECT_EQ(format("{}", 3.14), "3.14");
}

TEST(Format, FloatPrecision)
{
  EXPECT_EQ(format("{:.2f}", 3.14159), "3.14");
  EXPECT_EQ(format("{:.4f}", 3.14159), "3.1416");
  EXPECT_EQ(format("{:.0f}", 3.7), "4");
}

TEST(Format, FloatFixed)
{
  EXPECT_EQ(format("{:f}", 1000000.0), "1000000.000000");
  EXPECT_EQ(format("{:.2f}", 1000000.0), "1000000.00");
}

TEST(Format, FloatScientific)
{
  std::string result = format("{:e}", 1234567.0);
  EXPECT_TRUE(result.find('e') != std::string::npos);
}

TEST(Format, FloatWidth)
{
  EXPECT_EQ(format("{:10.2f}", 3.14), "      3.14");
}

TEST(Format, FloatZeroPad)
{
  EXPECT_EQ(format("{:010.2f}", 3.14), "0000003.14");
}

TEST(Format, FloatSign)
{
  EXPECT_EQ(format("{:+.2f}", 3.14), "+3.14");
  EXPECT_EQ(format("{:+.2f}", -3.14), "-3.14");
}

TEST(Format, FloatSpecialValues)
{
  EXPECT_EQ(format("{}", std::numeric_limits<double>::infinity()), "inf");
  EXPECT_EQ(format("{}", -std::numeric_limits<double>::infinity()), "-inf");
  EXPECT_EQ(format("{}", std::nan("")), "nan");
}

// ============================================================================
// Bool Formatting Tests
// ============================================================================

TEST(Format, BoolAsString)
{
  EXPECT_EQ(format("{}", true), "true");
  EXPECT_EQ(format("{}", false), "false");
}

TEST(Format, BoolAsNumber)
{
  EXPECT_EQ(format("{:d}", true), "1");
  EXPECT_EQ(format("{:d}", false), "0");
}

// ============================================================================
// Char Formatting Tests
// ============================================================================

TEST(Format, CharSimple)
{
  EXPECT_EQ(format("{}", 'A'), "A");
}

TEST(Format, CharAsNumber)
{
  EXPECT_EQ(format("{:d}", 'A'), "65");
  EXPECT_EQ(format("{:x}", 'A'), "41");
}

// ============================================================================
// Pointer Formatting Tests
// ============================================================================

TEST(Format, Pointer)
{
  int x    = 42;
  int* ptr = &x;
  std::string result = format("{}", static_cast<void*>(ptr));
  EXPECT_TRUE(result.find("0x") != std::string::npos || result == "(nil)");
}

TEST(Format, NullPointer)
{
  EXPECT_EQ(format("{}", nullptr), "(nil)");
}

// ============================================================================
// Format Spec Parsing Tests
// ============================================================================

TEST(FormatSpec, Parse_Empty)
{
  auto spec = parse_format_spec("");
  EXPECT_EQ(spec.fill, ' ');
  EXPECT_EQ(spec.align, '\0');
  EXPECT_EQ(spec.sign, '-');
  EXPECT_FALSE(spec.alt_form);
  EXPECT_FALSE(spec.zero_pad);
  EXPECT_EQ(spec.width, 0);
  EXPECT_EQ(spec.precision, -1);
  EXPECT_EQ(spec.type, '\0');
}

TEST(FormatSpec, Parse_Align)
{
  EXPECT_EQ(parse_format_spec("<").align, '<');
  EXPECT_EQ(parse_format_spec(">").align, '>');
  EXPECT_EQ(parse_format_spec("^").align, '^');
}

TEST(FormatSpec, Parse_Fill)
{
  auto spec = parse_format_spec("*<");
  EXPECT_EQ(spec.fill, '*');
  EXPECT_EQ(spec.align, '<');
}

TEST(FormatSpec, Parse_Sign)
{
  EXPECT_EQ(parse_format_spec("+").sign, '+');
  EXPECT_EQ(parse_format_spec(" ").sign, ' ');
}

TEST(FormatSpec, Parse_AltForm)
{
  EXPECT_TRUE(parse_format_spec("#").alt_form);
}

TEST(FormatSpec, Parse_ZeroPad)
{
  EXPECT_TRUE(parse_format_spec("0").zero_pad);
}

TEST(FormatSpec, Parse_Width)
{
  EXPECT_EQ(parse_format_spec("10").width, 10);
  EXPECT_EQ(parse_format_spec("100").width, 100);
}

TEST(FormatSpec, Parse_Precision)
{
  EXPECT_EQ(parse_format_spec(".5").precision, 5);
  EXPECT_EQ(parse_format_spec(".10").precision, 10);
}

TEST(FormatSpec, Parse_Type)
{
  EXPECT_EQ(parse_format_spec("d").type, 'd');
  EXPECT_EQ(parse_format_spec("x").type, 'x');
  EXPECT_EQ(parse_format_spec("f").type, 'f');
}

TEST(FormatSpec, Parse_Complex)
{
  auto spec = parse_format_spec("*>+#010.5f");
  EXPECT_EQ(spec.fill, '*');
  EXPECT_EQ(spec.align, '>');
  EXPECT_EQ(spec.sign, '+');
  EXPECT_TRUE(spec.alt_form);
  EXPECT_TRUE(spec.zero_pad);
  EXPECT_EQ(spec.width, 10);
  EXPECT_EQ(spec.precision, 5);
  EXPECT_EQ(spec.type, 'f');
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(Format, InvalidFormat_UnmatchedBrace)
{
  EXPECT_THROW(format("{"), std::invalid_argument);
}

TEST(Format, InvalidFormat_OutOfRange)
{
  EXPECT_THROW(format("{10}", 1), std::out_of_range);
}

TEST(Format, TryFormat_Success)
{
  auto result = try_format("Hello, {}!", "World");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "Hello, World!");
}

TEST(Format, TryFormat_Failure)
{
  auto result = try_format("{10}", 1);
  EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Complex Format Tests
// ============================================================================

TEST(Format, MixedTypes)
{
  EXPECT_EQ(format("{} is {} years old and {} tall",
                   "Alice", 30, 1.65),
            "Alice is 30 years old and 1.65 tall");
}

TEST(Format, ReusedIndices)
{
  EXPECT_EQ(format("{0}, {0}, {0}", "echo"), "echo, echo, echo");
}

TEST(Format, TableFormatting)
{
  std::string header = format("{:<10} {:>8} {:>8}", "Name", "Score", "Grade");
  EXPECT_EQ(header, "Name          Score    Grade");
}

TEST(Format, LargeNumbers)
{
  EXPECT_EQ(format("{}", 1234567890LL), "1234567890");
  EXPECT_EQ(format("{}", static_cast<unsigned long long>(18446744073709551615ULL)),
            "18446744073709551615");
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(Format, EmptyString)
{
  EXPECT_EQ(format("{}", ""), "");
  EXPECT_EQ(format("{:10}", ""), "          ");
}

TEST(Format, ZeroWidth)
{
  EXPECT_EQ(format("{:0}", "hello"), "hello");
}

TEST(Format, VeryLongString)
{
  std::string long_str(1000, 'x');
  EXPECT_EQ(format("{}", long_str), long_str);
}

TEST(Format, ManyArguments)
{
  EXPECT_EQ(format("{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9),
            "0123456789");
}

} // namespace fb
