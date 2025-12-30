/// @file test_string_builder.cpp
/// @brief Unit tests for the string_builder class

#include <gtest/gtest.h>

#include "fb/string_builder.h"

#include <cstdint>
#include <limits>

namespace fb::test
{

// ============================================================================
// Construction Tests
// ============================================================================

TEST(StringBuilderTest, DefaultConstruction)
{
  string_builder sb;
  EXPECT_TRUE(sb.empty());
  EXPECT_EQ(sb.size(), 0);
  EXPECT_EQ(sb.to_string(), "");
}

TEST(StringBuilderTest, ConstructionWithCapacity)
{
  string_builder sb(1024);
  EXPECT_TRUE(sb.empty());
  EXPECT_GE(sb.capacity(), 1024);
}

TEST(StringBuilderTest, MoveConstruction)
{
  string_builder sb1;
  sb1.append("Hello");

  string_builder sb2(std::move(sb1));
  EXPECT_EQ(sb2.to_string(), "Hello");
}

TEST(StringBuilderTest, MoveAssignment)
{
  string_builder sb1;
  sb1.append("Hello");

  string_builder sb2;
  sb2 = std::move(sb1);
  EXPECT_EQ(sb2.to_string(), "Hello");
}

// ============================================================================
// String Append Tests
// ============================================================================

TEST(StringBuilderTest, AppendStringView)
{
  string_builder sb;
  sb.append("Hello");
  EXPECT_EQ(sb.to_string(), "Hello");

  sb.append(", World!");
  EXPECT_EQ(sb.to_string(), "Hello, World!");
}

TEST(StringBuilderTest, AppendEmptyString)
{
  string_builder sb;
  sb.append("");
  EXPECT_TRUE(sb.empty());
}

TEST(StringBuilderTest, AppendChar)
{
  string_builder sb;
  sb.append('H').append('i');
  EXPECT_EQ(sb.to_string(), "Hi");
}

TEST(StringBuilderTest, AppendCharRepeated)
{
  string_builder sb;
  sb.append('-', std::size_t{5});
  EXPECT_EQ(sb.to_string(), "-----");
}

TEST(StringBuilderTest, AppendCharRepeatedZero)
{
  string_builder sb;
  sb.append('-', std::size_t{0});
  EXPECT_TRUE(sb.empty());
}

TEST(StringBuilderTest, AppendLine)
{
  string_builder sb;
  sb.append_line("Line 1");
  sb.append_line("Line 2");
  EXPECT_EQ(sb.to_string(), "Line 1\nLine 2\n");
}

TEST(StringBuilderTest, AppendLineEmpty)
{
  string_builder sb;
  sb.append_line();
  EXPECT_EQ(sb.to_string(), "\n");
}

// ============================================================================
// Numeric Append Tests
// ============================================================================

TEST(StringBuilderTest, AppendInt32)
{
  string_builder sb;
  sb.append_int(static_cast<std::int32_t>(42));
  EXPECT_EQ(sb.to_string(), "42");
}

TEST(StringBuilderTest, AppendInt32Negative)
{
  string_builder sb;
  sb.append_int(static_cast<std::int32_t>(-42));
  EXPECT_EQ(sb.to_string(), "-42");
}

TEST(StringBuilderTest, AppendInt32Zero)
{
  string_builder sb;
  sb.append_int(static_cast<std::int32_t>(0));
  EXPECT_EQ(sb.to_string(), "0");
}

TEST(StringBuilderTest, AppendInt32Limits)
{
  string_builder sb;
  sb.append_int(std::numeric_limits<std::int32_t>::max());
  EXPECT_EQ(sb.to_string(), "2147483647");

  sb.clear();
  sb.append_int(std::numeric_limits<std::int32_t>::min());
  EXPECT_EQ(sb.to_string(), "-2147483648");
}

TEST(StringBuilderTest, AppendInt64)
{
  string_builder sb;
  sb.append_int(static_cast<std::int64_t>(9223372036854775807LL));
  EXPECT_EQ(sb.to_string(), "9223372036854775807");
}

TEST(StringBuilderTest, AppendInt64Negative)
{
  string_builder sb;
  // Use literal to avoid overflow warning
  sb.append_int(std::numeric_limits<std::int64_t>::min());
  EXPECT_EQ(sb.to_string(), "-9223372036854775808");
}

TEST(StringBuilderTest, AppendUInt32)
{
  string_builder sb;
  sb.append_uint(static_cast<std::uint32_t>(4294967295U));
  EXPECT_EQ(sb.to_string(), "4294967295");
}

TEST(StringBuilderTest, AppendUInt64)
{
  string_builder sb;
  sb.append_uint(static_cast<std::uint64_t>(18446744073709551615ULL));
  EXPECT_EQ(sb.to_string(), "18446744073709551615");
}

TEST(StringBuilderTest, AppendDouble)
{
  string_builder sb;
  sb.append_double(3.14159, 2);
  EXPECT_EQ(sb.to_string(), "3.14");
}

TEST(StringBuilderTest, AppendDoubleDefaultPrecision)
{
  string_builder sb;
  sb.append_double(3.141592);
  EXPECT_EQ(sb.to_string(), "3.141592");
}

TEST(StringBuilderTest, AppendDoubleNegative)
{
  string_builder sb;
  sb.append_double(-123.456, 3);
  EXPECT_EQ(sb.to_string(), "-123.456");
}

TEST(StringBuilderTest, AppendDoubleZero)
{
  string_builder sb;
  sb.append_double(0.0, 2);
  EXPECT_EQ(sb.to_string(), "0.00");
}

TEST(StringBuilderTest, AppendBoolTrue)
{
  string_builder sb;
  sb.append_bool(true);
  EXPECT_EQ(sb.to_string(), "true");
}

TEST(StringBuilderTest, AppendBoolFalse)
{
  string_builder sb;
  sb.append_bool(false);
  EXPECT_EQ(sb.to_string(), "false");
}

// ============================================================================
// Stream Operator Tests
// ============================================================================

TEST(StringBuilderTest, StreamOperatorString)
{
  string_builder sb;
  sb << "Hello" << ", " << "World!";
  EXPECT_EQ(sb.to_string(), "Hello, World!");
}

TEST(StringBuilderTest, StreamOperatorChar)
{
  string_builder sb;
  sb << 'A' << 'B' << 'C';
  EXPECT_EQ(sb.to_string(), "ABC");
}

TEST(StringBuilderTest, StreamOperatorCString)
{
  string_builder    sb;
  const char* const cstr = "C-String";
  sb << cstr;
  EXPECT_EQ(sb.to_string(), "C-String");
}

TEST(StringBuilderTest, StreamOperatorInt32)
{
  string_builder sb;
  sb << static_cast<std::int32_t>(42);
  EXPECT_EQ(sb.to_string(), "42");
}

TEST(StringBuilderTest, StreamOperatorInt64)
{
  string_builder sb;
  sb << static_cast<std::int64_t>(123456789012345LL);
  EXPECT_EQ(sb.to_string(), "123456789012345");
}

TEST(StringBuilderTest, StreamOperatorUInt32)
{
  string_builder sb;
  sb << static_cast<std::uint32_t>(4000000000U);
  EXPECT_EQ(sb.to_string(), "4000000000");
}

TEST(StringBuilderTest, StreamOperatorUInt64)
{
  string_builder sb;
  sb << static_cast<std::uint64_t>(10000000000000000000ULL);
  EXPECT_EQ(sb.to_string(), "10000000000000000000");
}

TEST(StringBuilderTest, StreamOperatorDouble)
{
  string_builder sb;
  sb << 3.14;
  // Default precision is 6
  EXPECT_EQ(sb.to_string(), "3.140000");
}

TEST(StringBuilderTest, StreamOperatorBool)
{
  string_builder sb;
  sb << true << " and " << false;
  EXPECT_EQ(sb.to_string(), "true and false");
}

TEST(StringBuilderTest, StreamOperatorMixed)
{
  string_builder sb;
  sb << "Count: " << static_cast<std::int32_t>(42) << ", Active: " << true;
  EXPECT_EQ(sb.to_string(), "Count: 42, Active: true");
}

// ============================================================================
// Method Chaining Tests
// ============================================================================

TEST(StringBuilderTest, MethodChaining)
{
  string_builder sb;
  sb.append("Name: ")
      .append("John")
      .append_line()
      .append("Age: ")
      .append_int(static_cast<std::int32_t>(30))
      .append_line();

  EXPECT_EQ(sb.to_string(), "Name: John\nAge: 30\n");
}

TEST(StringBuilderTest, ComplexChaining)
{
  string_builder sb;
  sb.append("Header")
      .append_line()
      .append('-', std::size_t{20})
      .append_line()
      .append("Value: ")
      .append_double(3.14159, 4)
      .append_line()
      .append("Done: ")
      .append_bool(true);

  std::string expected = "Header\n--------------------\nValue: 3.1416\nDone: true";
  EXPECT_EQ(sb.to_string(), expected);
}

// ============================================================================
// Output Method Tests
// ============================================================================

TEST(StringBuilderTest, ToStringCopy)
{
  string_builder sb;
  sb.append("Hello");

  std::string s1 = sb.to_string();
  std::string s2 = sb.to_string();

  // Both should be equal
  EXPECT_EQ(s1, s2);

  // Modifying s1 should not affect sb
  s1[0] = 'J';
  EXPECT_EQ(sb.to_string(), "Hello");
}

TEST(StringBuilderTest, ViewReturnsValidView)
{
  string_builder sb;
  sb.append("Hello");

  std::string_view view = sb.view();
  EXPECT_EQ(view, "Hello");
}

TEST(StringBuilderTest, ViewInvalidatedByAppend)
{
  string_builder sb;
  sb.reserve(5); // Small capacity to force reallocation
  sb.append("Hi");

  std::string_view view1 = sb.view();
  EXPECT_EQ(view1, "Hi");

  // This may reallocate the buffer
  sb.append("LongStringThatMayCauseReallocation");

  // Original view may be invalidated, but content is correct
  std::string_view view2 = sb.view();
  EXPECT_EQ(view2, "HiLongStringThatMayCauseReallocation");
}

// ============================================================================
// Management Method Tests
// ============================================================================

TEST(StringBuilderTest, Clear)
{
  string_builder sb;
  sb.append("Hello");
  EXPECT_FALSE(sb.empty());

  sb.clear();
  EXPECT_TRUE(sb.empty());
  EXPECT_EQ(sb.size(), 0);
  EXPECT_EQ(sb.to_string(), "");
}

TEST(StringBuilderTest, ClearPreservesCapacity)
{
  string_builder sb(1024);
  sb.append("Hello");
  std::size_t cap_before = sb.capacity();

  sb.clear();
  std::size_t cap_after = sb.capacity();

  EXPECT_GE(cap_after, cap_before);
}

TEST(StringBuilderTest, Reserve)
{
  string_builder sb;
  sb.reserve(1000);
  EXPECT_GE(sb.capacity(), 1000);
}

TEST(StringBuilderTest, ReserveDoesNotShrink)
{
  string_builder sb;
  sb.reserve(1000);
  std::size_t cap1 = sb.capacity();

  sb.reserve(100);
  std::size_t cap2 = sb.capacity();

  EXPECT_GE(cap2, cap1);
}

TEST(StringBuilderTest, Size)
{
  string_builder sb;
  EXPECT_EQ(sb.size(), 0);

  sb.append("Hello");
  EXPECT_EQ(sb.size(), 5);

  sb.append(" World");
  EXPECT_EQ(sb.size(), 11);
}

TEST(StringBuilderTest, Empty)
{
  string_builder sb;
  EXPECT_TRUE(sb.empty());

  sb.append("x");
  EXPECT_FALSE(sb.empty());

  sb.clear();
  EXPECT_TRUE(sb.empty());
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(StringBuilderTest, LargeAppend)
{
  string_builder sb;
  std::string    large_string(10000, 'X');
  sb.append(large_string);
  EXPECT_EQ(sb.size(), 10000);
  EXPECT_EQ(sb.to_string(), large_string);
}

TEST(StringBuilderTest, ManySmallAppends)
{
  string_builder sb;
  for (int i = 0; i < 1000; ++i)
  {
    sb.append("X");
  }
  EXPECT_EQ(sb.size(), 1000);
}

TEST(StringBuilderTest, AppendAfterClear)
{
  string_builder sb;
  sb.append("First");
  sb.clear();
  sb.append("Second");
  EXPECT_EQ(sb.to_string(), "Second");
}

// ============================================================================
// Constructor with Initial Content Tests
// ============================================================================

TEST(StringBuilderTest, ConstructionWithInitialContent)
{
  string_builder sb("Hello");
  EXPECT_EQ(sb.to_string(), "Hello");
  EXPECT_EQ(sb.size(), 5);
}

TEST(StringBuilderTest, ConstructionWithEmptyString)
{
  string_builder sb("");
  EXPECT_TRUE(sb.empty());
}

TEST(StringBuilderTest, ConstructionWithInitialThenAppend)
{
  string_builder sb("Hello");
  sb.append(", World!");
  EXPECT_EQ(sb.to_string(), "Hello, World!");
}

// ============================================================================
// Modification Operation Tests
// ============================================================================

TEST(StringBuilderTest, InsertAtBeginning)
{
  string_builder sb;
  sb.append("World");
  sb.insert(0, "Hello ");
  EXPECT_EQ(sb.to_string(), "Hello World");
}

TEST(StringBuilderTest, InsertAtMiddle)
{
  string_builder sb;
  sb.append("HelloWorld");
  sb.insert(5, ", ");
  EXPECT_EQ(sb.to_string(), "Hello, World");
}

TEST(StringBuilderTest, InsertAtEnd)
{
  string_builder sb;
  sb.append("Hello");
  sb.insert(100, " World"); // Position beyond size - should clamp
  EXPECT_EQ(sb.to_string(), "Hello World");
}

TEST(StringBuilderTest, RemoveFromBeginning)
{
  string_builder sb;
  sb.append("Hello World");
  sb.remove(0, 6);
  EXPECT_EQ(sb.to_string(), "World");
}

TEST(StringBuilderTest, RemoveFromMiddle)
{
  string_builder sb;
  sb.append("Hello, World");
  sb.remove(5, 2);
  EXPECT_EQ(sb.to_string(), "HelloWorld");
}

TEST(StringBuilderTest, RemoveToEnd)
{
  string_builder sb;
  sb.append("Hello World");
  sb.remove(5);
  EXPECT_EQ(sb.to_string(), "Hello");
}

TEST(StringBuilderTest, RemoveOutOfRange)
{
  string_builder sb;
  sb.append("Hello");
  sb.remove(100, 5); // Position beyond size - should do nothing
  EXPECT_EQ(sb.to_string(), "Hello");
}

TEST(StringBuilderTest, ReplaceFirst)
{
  string_builder sb;
  sb.append("Hello World World");
  sb.replace("World", "Universe");
  EXPECT_EQ(sb.to_string(), "Hello Universe World");
}

TEST(StringBuilderTest, ReplaceFirstNotFound)
{
  string_builder sb;
  sb.append("Hello World");
  sb.replace("Foo", "Bar");
  EXPECT_EQ(sb.to_string(), "Hello World");
}

TEST(StringBuilderTest, ReplaceAll)
{
  string_builder sb;
  sb.append("Hello World World World");
  sb.replace_all("World", "Universe");
  EXPECT_EQ(sb.to_string(), "Hello Universe Universe Universe");
}

TEST(StringBuilderTest, ReplaceAllNotFound)
{
  string_builder sb;
  sb.append("Hello World");
  sb.replace_all("Foo", "Bar");
  EXPECT_EQ(sb.to_string(), "Hello World");
}

TEST(StringBuilderTest, ReplaceAllEmpty)
{
  string_builder sb;
  sb.append("Hello World");
  sb.replace_all("", "X"); // Empty 'from' should do nothing
  EXPECT_EQ(sb.to_string(), "Hello World");
}

TEST(StringBuilderTest, ReplaceAllWithShorter)
{
  string_builder sb;
  sb.append("aaa");
  sb.replace_all("aa", "b");
  EXPECT_EQ(sb.to_string(), "ba");
}

TEST(StringBuilderTest, ReplaceAllWithLonger)
{
  string_builder sb;
  sb.append("a_a_a");
  sb.replace_all("_", "--");
  EXPECT_EQ(sb.to_string(), "a--a--a");
}

// ============================================================================
// Append Format Tests
// ============================================================================

TEST(StringBuilderTest, AppendFormatBasic)
{
  string_builder sb;
  sb.append_format("Hello, {}!", "World");
  EXPECT_EQ(sb.to_string(), "Hello, World!");
}

TEST(StringBuilderTest, AppendFormatMultipleArgs)
{
  string_builder sb;
  sb.append_format("{} + {} = {}", 1, 2, 3);
  EXPECT_EQ(sb.to_string(), "1 + 2 = 3");
}

TEST(StringBuilderTest, AppendFormatChained)
{
  string_builder sb;
  sb.append("Values: ")
      .append_format("{}, {}", 10, 20)
      .append_line()
      .append_format("Sum: {}", 30);
  EXPECT_EQ(sb.to_string(), "Values: 10, 20\nSum: 30");
}

// ============================================================================
// Clear Returns This Tests
// ============================================================================

TEST(StringBuilderTest, ClearChaining)
{
  string_builder sb;
  sb.append("Hello").clear().append("World");
  EXPECT_EQ(sb.to_string(), "World");
}

// ============================================================================
// Modification Chaining Tests
// ============================================================================

TEST(StringBuilderTest, ModificationChaining)
{
  string_builder sb;
  sb.append("Hello")
      .insert(5, " Beautiful")
      .append(" World")
      .replace("Beautiful", "Wonderful");
  EXPECT_EQ(sb.to_string(), "Hello Wonderful World");
}

} // namespace fb::test
