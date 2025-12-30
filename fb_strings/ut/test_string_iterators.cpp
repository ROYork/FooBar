/// @file test_string_iterators.cpp
/// @brief Unit tests for string iterator classes

#include <gtest/gtest.h>

#include "fb/string_iterators.h"

#include <string>
#include <vector>

namespace fb::test
{

// ============================================================================
// Helper Functions
// ============================================================================

template <typename Iterator>
std::vector<std::string> collect(Iterator it)
{
  std::vector<std::string> result;
  while (it.has_next())
  {
    result.emplace_back(it.next());
  }
  return result;
}

// ============================================================================
// line_iterator Tests
// ============================================================================

TEST(LineIteratorTest, EmptyString)
{
  line_iterator it("");
  EXPECT_TRUE(it.has_next());

  auto result = it.next();
  EXPECT_EQ(result, "");
  EXPECT_FALSE(it.has_next());
}

TEST(LineIteratorTest, SingleLineNoNewline)
{
  line_iterator it("Hello");
  EXPECT_TRUE(it.has_next());

  EXPECT_EQ(it.next(), "Hello");
  EXPECT_FALSE(it.has_next());
}

TEST(LineIteratorTest, SingleLineWithNewline)
{
  line_iterator it("Hello\n");
  EXPECT_TRUE(it.has_next());

  EXPECT_EQ(it.next(), "Hello");
  EXPECT_TRUE(it.has_next());

  EXPECT_EQ(it.next(), "");
  EXPECT_FALSE(it.has_next());
}

TEST(LineIteratorTest, MultipleLines)
{
  auto lines = collect(line_iterator("Line1\nLine2\nLine3"));
  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "Line1");
  EXPECT_EQ(lines[1], "Line2");
  EXPECT_EQ(lines[2], "Line3");
}

TEST(LineIteratorTest, WindowsLineEndings)
{
  auto lines = collect(line_iterator("Line1\r\nLine2\r\nLine3"));
  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "Line1");
  EXPECT_EQ(lines[1], "Line2");
  EXPECT_EQ(lines[2], "Line3");
}

TEST(LineIteratorTest, MacLineEndings)
{
  auto lines = collect(line_iterator("Line1\rLine2\rLine3"));
  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "Line1");
  EXPECT_EQ(lines[1], "Line2");
  EXPECT_EQ(lines[2], "Line3");
}

TEST(LineIteratorTest, MixedLineEndings)
{
  auto lines = collect(line_iterator("Unix\nWindows\r\nMac\rEnd"));
  EXPECT_EQ(lines.size(), 4);
  EXPECT_EQ(lines[0], "Unix");
  EXPECT_EQ(lines[1], "Windows");
  EXPECT_EQ(lines[2], "Mac");
  EXPECT_EQ(lines[3], "End");
}

TEST(LineIteratorTest, EmptyLines)
{
  auto lines = collect(line_iterator("A\n\nB"));
  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "A");
  EXPECT_EQ(lines[1], "");
  EXPECT_EQ(lines[2], "B");
}

TEST(LineIteratorTest, OnlyNewlines)
{
  auto lines = collect(line_iterator("\n\n"));
  EXPECT_EQ(lines.size(), 3);
  EXPECT_EQ(lines[0], "");
  EXPECT_EQ(lines[1], "");
  EXPECT_EQ(lines[2], "");
}

TEST(LineIteratorTest, Reset)
{
  line_iterator it("A\nB");

  EXPECT_EQ(it.next(), "A");
  EXPECT_EQ(it.next(), "B");
  EXPECT_FALSE(it.has_next());

  it.reset();

  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "A");
  EXPECT_EQ(it.next(), "B");
}

// ============================================================================
// word_iterator Tests
// ============================================================================

TEST(WordIteratorTest, EmptyString)
{
  word_iterator it("");
  EXPECT_FALSE(it.has_next());
}

TEST(WordIteratorTest, SingleWord)
{
  word_iterator it("Hello");
  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "Hello");
  EXPECT_FALSE(it.has_next());
}

TEST(WordIteratorTest, MultipleWords)
{
  auto words = collect(word_iterator("Hello World Foo Bar"));
  EXPECT_EQ(words.size(), 4);
  EXPECT_EQ(words[0], "Hello");
  EXPECT_EQ(words[1], "World");
  EXPECT_EQ(words[2], "Foo");
  EXPECT_EQ(words[3], "Bar");
}

TEST(WordIteratorTest, LeadingWhitespace)
{
  auto words = collect(word_iterator("   Hello"));
  EXPECT_EQ(words.size(), 1);
  EXPECT_EQ(words[0], "Hello");
}

TEST(WordIteratorTest, TrailingWhitespace)
{
  auto words = collect(word_iterator("Hello   "));
  EXPECT_EQ(words.size(), 1);
  EXPECT_EQ(words[0], "Hello");
}

TEST(WordIteratorTest, MultipleSpaces)
{
  auto words = collect(word_iterator("Hello    World"));
  EXPECT_EQ(words.size(), 2);
  EXPECT_EQ(words[0], "Hello");
  EXPECT_EQ(words[1], "World");
}

TEST(WordIteratorTest, MixedWhitespace)
{
  auto words = collect(word_iterator("Hello\tWorld\nFoo\r\nBar"));
  EXPECT_EQ(words.size(), 4);
  EXPECT_EQ(words[0], "Hello");
  EXPECT_EQ(words[1], "World");
  EXPECT_EQ(words[2], "Foo");
  EXPECT_EQ(words[3], "Bar");
}

TEST(WordIteratorTest, OnlyWhitespace)
{
  word_iterator it("   \t\n  ");
  EXPECT_FALSE(it.has_next());
}

TEST(WordIteratorTest, Reset)
{
  word_iterator it("Hello World");

  EXPECT_EQ(it.next(), "Hello");
  EXPECT_EQ(it.next(), "World");
  EXPECT_FALSE(it.has_next());

  it.reset();

  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "Hello");
}

// ============================================================================
// token_iterator Tests
// ============================================================================

TEST(TokenIteratorTest, EmptyString)
{
  token_iterator it("", ",");
  EXPECT_FALSE(it.has_next());
}

TEST(TokenIteratorTest, SingleToken)
{
  token_iterator it("Hello", ",");
  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "Hello");
  EXPECT_FALSE(it.has_next());
}

TEST(TokenIteratorTest, MultipleTokens)
{
  auto tokens = collect(token_iterator("A,B,C", ","));
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "A");
  EXPECT_EQ(tokens[1], "B");
  EXPECT_EQ(tokens[2], "C");
}

TEST(TokenIteratorTest, EmptyTokens)
{
  auto tokens = collect(token_iterator("A,,B", ","));
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "A");
  EXPECT_EQ(tokens[1], "");
  EXPECT_EQ(tokens[2], "B");
}

TEST(TokenIteratorTest, LeadingDelimiter)
{
  auto tokens = collect(token_iterator(",A,B", ","));
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "");
  EXPECT_EQ(tokens[1], "A");
  EXPECT_EQ(tokens[2], "B");
}

TEST(TokenIteratorTest, TrailingDelimiter)
{
  auto tokens = collect(token_iterator("A,B,", ","));
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "A");
  EXPECT_EQ(tokens[1], "B");
  EXPECT_EQ(tokens[2], "");
}

TEST(TokenIteratorTest, MultipleDelimiters)
{
  auto tokens = collect(token_iterator("A;B,C:D", ";,:"));
  EXPECT_EQ(tokens.size(), 4);
  EXPECT_EQ(tokens[0], "A");
  EXPECT_EQ(tokens[1], "B");
  EXPECT_EQ(tokens[2], "C");
  EXPECT_EQ(tokens[3], "D");
}

TEST(TokenIteratorTest, OnlyDelimiters)
{
  auto tokens = collect(token_iterator(",,", ","));
  EXPECT_EQ(tokens.size(), 3);
  EXPECT_EQ(tokens[0], "");
  EXPECT_EQ(tokens[1], "");
  EXPECT_EQ(tokens[2], "");
}

TEST(TokenIteratorTest, NoDelimitersInString)
{
  auto tokens = collect(token_iterator("Hello World", ","));
  EXPECT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0], "Hello World");
}

TEST(TokenIteratorTest, Reset)
{
  token_iterator it("A,B,C", ",");

  EXPECT_EQ(it.next(), "A");
  EXPECT_EQ(it.next(), "B");
  it.reset();

  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "A");
}

// ============================================================================
// codepoint_iterator Tests
// ============================================================================

TEST(CodepointIteratorTest, EmptyString)
{
  codepoint_iterator it("");
  EXPECT_FALSE(it.has_next());
}

TEST(CodepointIteratorTest, AsciiString)
{
  auto cps = collect(codepoint_iterator("Hello"));
  EXPECT_EQ(cps.size(), 5);
  EXPECT_EQ(cps[0], "H");
  EXPECT_EQ(cps[1], "e");
  EXPECT_EQ(cps[2], "l");
  EXPECT_EQ(cps[3], "l");
  EXPECT_EQ(cps[4], "o");
}

TEST(CodepointIteratorTest, TwoByteCharacters)
{
  // "Über" - Ü is 2 bytes
  std::string str = "\xC3\x9C" "ber";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 4);
  EXPECT_EQ(cps[0], "\xC3\x9C"); // Ü
  EXPECT_EQ(cps[1], "b");
  EXPECT_EQ(cps[2], "e");
  EXPECT_EQ(cps[3], "r");
}

TEST(CodepointIteratorTest, ThreeByteCharacters)
{
  // "日本語" - each character is 3 bytes
  std::string str = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 3);
  EXPECT_EQ(cps[0], "\xE6\x97\xA5"); // 日
  EXPECT_EQ(cps[1], "\xE6\x9C\xAC"); // 本
  EXPECT_EQ(cps[2], "\xE8\xAA\x9E"); // 語
}

TEST(CodepointIteratorTest, FourByteCharacter)
{
  // 😀 is 4 bytes
  std::string str = "\xF0\x9F\x98\x80";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 1);
  EXPECT_EQ(cps[0], str);
}

TEST(CodepointIteratorTest, MixedCharacters)
{
  // "Hello, 世界" - mixed ASCII and 3-byte
  std::string str = "Hello, \xE4\xB8\x96\xE7\x95\x8C";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 9);
  EXPECT_EQ(cps[0], "H");
  EXPECT_EQ(cps[6], " ");
  EXPECT_EQ(cps[7], "\xE4\xB8\x96"); // 世
  EXPECT_EQ(cps[8], "\xE7\x95\x8C"); // 界
}

TEST(CodepointIteratorTest, InvalidContinuationByte)
{
  // Invalid: continuation byte without start
  std::string str = "A\x80";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 2);
  EXPECT_EQ(cps[0], "A");
  EXPECT_EQ(cps[1], "\x80"); // Invalid byte returned as single
}

TEST(CodepointIteratorTest, TruncatedSequence)
{
  // Truncated 2-byte sequence
  std::string str = "A\xC3";
  auto        cps = collect(codepoint_iterator(str));

  EXPECT_EQ(cps.size(), 2);
  EXPECT_EQ(cps[0], "A");
  EXPECT_EQ(cps[1], "\xC3"); // Truncated, returned as single
}

TEST(CodepointIteratorTest, Reset)
{
  codepoint_iterator it("AB");

  EXPECT_EQ(it.next(), "A");
  EXPECT_EQ(it.next(), "B");
  EXPECT_FALSE(it.has_next());

  it.reset();

  EXPECT_TRUE(it.has_next());
  EXPECT_EQ(it.next(), "A");
}

// ============================================================================
// Zero-Copy Verification Tests
// ============================================================================

TEST(IteratorZeroCopyTest, LineIteratorReturnsViews)
{
  std::string       str = "Hello\nWorld";
  line_iterator     it(str);
  std::string_view  line = it.next();

  // Verify it's pointing into original string
  EXPECT_GE(line.data(), str.data());
  EXPECT_LE(line.data() + line.size(), str.data() + str.size());
}

TEST(IteratorZeroCopyTest, WordIteratorReturnsViews)
{
  std::string      str  = "Hello World";
  word_iterator    it(str);
  std::string_view word = it.next();

  EXPECT_GE(word.data(), str.data());
  EXPECT_LE(word.data() + word.size(), str.data() + str.size());
}

TEST(IteratorZeroCopyTest, TokenIteratorReturnsViews)
{
  std::string       str   = "A,B,C";
  token_iterator    it(str, ",");
  std::string_view  token = it.next();

  EXPECT_GE(token.data(), str.data());
  EXPECT_LE(token.data() + token.size(), str.data() + str.size());
}

TEST(IteratorZeroCopyTest, CodepointIteratorReturnsViews)
{
  std::string          str = "Hello";
  codepoint_iterator   it(str);
  std::string_view     cp = it.next();

  EXPECT_GE(cp.data(), str.data());
  EXPECT_LE(cp.data() + cp.size(), str.data() + str.size());
}

} // namespace fb::test
