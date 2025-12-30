/// @file test_utf8_utils.cpp
/// @brief Unit tests for UTF-8 string manipulation utilities

#include <gtest/gtest.h>

#include "fb/utf8_utils.h"

#include <string>

namespace fb::test
{

// ============================================================================
// Test Data
// ============================================================================

// ASCII string
const std::string ASCII_STR = "Hello";

// Mixed ASCII and multi-byte: "Hello, 世界" (Hello, World in Chinese)
const std::string MIXED_STR = "Hello, \xE4\xB8\x96\xE7\x95\x8C";

// 2-byte characters: "Über" (German)
const std::string TWO_BYTE_STR = "\xC3\x9C" "ber";

// 3-byte characters: "日本語" (Japanese)
const std::string THREE_BYTE_STR = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";

// 4-byte character: emoji 😀 (U+1F600)
const std::string FOUR_BYTE_STR = "\xF0\x9F\x98\x80";

// ============================================================================
// utf8_length Tests
// ============================================================================

TEST(Utf8UtilsTest, LengthEmptyString)
{
  EXPECT_EQ(utf8_length(""), 0);
}

TEST(Utf8UtilsTest, LengthAsciiString)
{
  EXPECT_EQ(utf8_length(ASCII_STR), 5);
}

TEST(Utf8UtilsTest, LengthMixedString)
{
  // "Hello, " = 7 + "世界" = 2 = 9 code points
  EXPECT_EQ(utf8_length(MIXED_STR), 9);
}

TEST(Utf8UtilsTest, LengthTwoByteChars)
{
  // Ü = 1 + ber = 3 = 4 code points
  EXPECT_EQ(utf8_length(TWO_BYTE_STR), 4);
}

TEST(Utf8UtilsTest, LengthThreeByteChars)
{
  // 日本語 = 3 code points
  EXPECT_EQ(utf8_length(THREE_BYTE_STR), 3);
}

TEST(Utf8UtilsTest, LengthFourByteChar)
{
  // 😀 = 1 code point
  EXPECT_EQ(utf8_length(FOUR_BYTE_STR), 1);
}

// ============================================================================
// utf8_at Tests
// ============================================================================

TEST(Utf8UtilsTest, AtAscii)
{
  EXPECT_EQ(utf8_at(ASCII_STR, 0), "H");
  EXPECT_EQ(utf8_at(ASCII_STR, 4), "o");
}

TEST(Utf8UtilsTest, AtMixedString)
{
  EXPECT_EQ(utf8_at(MIXED_STR, 0), "H");
  EXPECT_EQ(utf8_at(MIXED_STR, 7), "\xE4\xB8\x96"); // 世
  EXPECT_EQ(utf8_at(MIXED_STR, 8), "\xE7\x95\x8C"); // 界
}

TEST(Utf8UtilsTest, AtThreeByteChars)
{
  EXPECT_EQ(utf8_at(THREE_BYTE_STR, 0), "\xE6\x97\xA5"); // 日
  EXPECT_EQ(utf8_at(THREE_BYTE_STR, 1), "\xE6\x9C\xAC"); // 本
  EXPECT_EQ(utf8_at(THREE_BYTE_STR, 2), "\xE8\xAA\x9E"); // 語
}

TEST(Utf8UtilsTest, AtFourByteChar)
{
  EXPECT_EQ(utf8_at(FOUR_BYTE_STR, 0), FOUR_BYTE_STR);
}

TEST(Utf8UtilsTest, AtOutOfRange)
{
  EXPECT_EQ(utf8_at(ASCII_STR, 100), "");
  EXPECT_EQ(utf8_at("", 0), "");
}

// ============================================================================
// utf8_left Tests
// ============================================================================

TEST(Utf8UtilsTest, LeftAscii)
{
  EXPECT_EQ(utf8_left(ASCII_STR, 0), "");
  EXPECT_EQ(utf8_left(ASCII_STR, 3), "Hel");
  EXPECT_EQ(utf8_left(ASCII_STR, 5), "Hello");
  EXPECT_EQ(utf8_left(ASCII_STR, 100), "Hello");
}

TEST(Utf8UtilsTest, LeftMixed)
{
  EXPECT_EQ(utf8_left(MIXED_STR, 7), "Hello, ");
  EXPECT_EQ(utf8_left(MIXED_STR, 8), "Hello, \xE4\xB8\x96");
  EXPECT_EQ(utf8_left(MIXED_STR, 9), MIXED_STR);
}

TEST(Utf8UtilsTest, LeftThreeByte)
{
  EXPECT_EQ(utf8_left(THREE_BYTE_STR, 1), "\xE6\x97\xA5");
  EXPECT_EQ(utf8_left(THREE_BYTE_STR, 2), "\xE6\x97\xA5\xE6\x9C\xAC");
}

TEST(Utf8UtilsTest, LeftFourByte)
{
  std::string two_emoji = FOUR_BYTE_STR + FOUR_BYTE_STR;
  EXPECT_EQ(utf8_left(two_emoji, 1), FOUR_BYTE_STR);
}

TEST(Utf8UtilsTest, LeftEmpty)
{
  EXPECT_EQ(utf8_left("", 5), "");
}

// ============================================================================
// utf8_right Tests
// ============================================================================

TEST(Utf8UtilsTest, RightAscii)
{
  EXPECT_EQ(utf8_right(ASCII_STR, 0), "");
  EXPECT_EQ(utf8_right(ASCII_STR, 2), "lo");
  EXPECT_EQ(utf8_right(ASCII_STR, 5), "Hello");
  EXPECT_EQ(utf8_right(ASCII_STR, 100), "Hello");
}

TEST(Utf8UtilsTest, RightMixed)
{
  EXPECT_EQ(utf8_right(MIXED_STR, 2), "\xE4\xB8\x96\xE7\x95\x8C"); // 世界
  EXPECT_EQ(utf8_right(MIXED_STR, 3), " \xE4\xB8\x96\xE7\x95\x8C"); // space + 世界
}

TEST(Utf8UtilsTest, RightThreeByte)
{
  EXPECT_EQ(utf8_right(THREE_BYTE_STR, 1), "\xE8\xAA\x9E"); // 語
  EXPECT_EQ(utf8_right(THREE_BYTE_STR, 2), "\xE6\x9C\xAC\xE8\xAA\x9E");
}

TEST(Utf8UtilsTest, RightEmpty)
{
  EXPECT_EQ(utf8_right("", 5), "");
}

// ============================================================================
// utf8_mid Tests
// ============================================================================

TEST(Utf8UtilsTest, MidAscii)
{
  EXPECT_EQ(utf8_mid(ASCII_STR, 1, 3), "ell");
  EXPECT_EQ(utf8_mid(ASCII_STR, 0), "Hello");
  EXPECT_EQ(utf8_mid(ASCII_STR, 2), "llo");
}

TEST(Utf8UtilsTest, MidMixed)
{
  EXPECT_EQ(utf8_mid(MIXED_STR, 7, 1), "\xE4\xB8\x96"); // 世
  EXPECT_EQ(utf8_mid(MIXED_STR, 7, 2), "\xE4\xB8\x96\xE7\x95\x8C");
  EXPECT_EQ(utf8_mid(MIXED_STR, 5, 3), ", \xE4\xB8\x96");
}

TEST(Utf8UtilsTest, MidThreeByte)
{
  EXPECT_EQ(utf8_mid(THREE_BYTE_STR, 1, 1), "\xE6\x9C\xAC"); // 本
}

TEST(Utf8UtilsTest, MidOutOfRange)
{
  EXPECT_EQ(utf8_mid(ASCII_STR, 10), "");
  EXPECT_EQ(utf8_mid(ASCII_STR, 3, 100), "lo");
}

TEST(Utf8UtilsTest, MidEmpty)
{
  EXPECT_EQ(utf8_mid("", 0), "");
}

// ============================================================================
// utf8_byte_offset Tests
// ============================================================================

TEST(Utf8UtilsTest, ByteOffsetAscii)
{
  EXPECT_EQ(utf8_byte_offset(ASCII_STR, 0), 0);
  EXPECT_EQ(utf8_byte_offset(ASCII_STR, 3), 3);
  EXPECT_EQ(utf8_byte_offset(ASCII_STR, 5), 5);
}

TEST(Utf8UtilsTest, ByteOffsetMixed)
{
  EXPECT_EQ(utf8_byte_offset(MIXED_STR, 7), 7);  // After "Hello, "
  EXPECT_EQ(utf8_byte_offset(MIXED_STR, 8), 10); // After first 3-byte char
  EXPECT_EQ(utf8_byte_offset(MIXED_STR, 9), 13); // End
}

TEST(Utf8UtilsTest, ByteOffsetOutOfRange)
{
  EXPECT_EQ(utf8_byte_offset(ASCII_STR, 100), std::string::npos);
}

// ============================================================================
// utf8_codepoint_index Tests
// ============================================================================

TEST(Utf8UtilsTest, CodepointIndexAscii)
{
  EXPECT_EQ(utf8_codepoint_index(ASCII_STR, 0), 0);
  EXPECT_EQ(utf8_codepoint_index(ASCII_STR, 3), 3);
}

TEST(Utf8UtilsTest, CodepointIndexMixed)
{
  EXPECT_EQ(utf8_codepoint_index(MIXED_STR, 7), 7);  // "Hello, " = 7 cps
  EXPECT_EQ(utf8_codepoint_index(MIXED_STR, 10), 8); // After 世 (3 bytes)
}

TEST(Utf8UtilsTest, CodepointIndexOutOfRange)
{
  EXPECT_EQ(utf8_codepoint_index(ASCII_STR, 100), std::string::npos);
}

// ============================================================================
// is_valid_utf8 Tests
// ============================================================================

TEST(Utf8UtilsTest, ValidUtf8Empty)
{
  EXPECT_TRUE(is_valid_utf8(""));
}

TEST(Utf8UtilsTest, ValidUtf8Ascii)
{
  EXPECT_TRUE(is_valid_utf8(ASCII_STR));
  EXPECT_TRUE(is_valid_utf8("0123456789"));
  EXPECT_TRUE(is_valid_utf8("!@#$%^&*()"));
}

TEST(Utf8UtilsTest, ValidUtf8MultiByte)
{
  EXPECT_TRUE(is_valid_utf8(MIXED_STR));
  EXPECT_TRUE(is_valid_utf8(TWO_BYTE_STR));
  EXPECT_TRUE(is_valid_utf8(THREE_BYTE_STR));
  EXPECT_TRUE(is_valid_utf8(FOUR_BYTE_STR));
}

TEST(Utf8UtilsTest, InvalidUtf8ContinuationByte)
{
  // Orphan continuation byte
  EXPECT_FALSE(is_valid_utf8("\x80"));
  EXPECT_FALSE(is_valid_utf8("\xBF"));
}

TEST(Utf8UtilsTest, InvalidUtf8TruncatedSequence)
{
  // Truncated 2-byte sequence
  EXPECT_FALSE(is_valid_utf8("\xC3"));

  // Truncated 3-byte sequence
  EXPECT_FALSE(is_valid_utf8("\xE4\xB8"));

  // Truncated 4-byte sequence
  EXPECT_FALSE(is_valid_utf8("\xF0\x9F\x98"));
}

TEST(Utf8UtilsTest, InvalidUtf8OverlongEncoding)
{
  // Overlong encoding of NUL (should be 0x00)
  EXPECT_FALSE(is_valid_utf8("\xC0\x80"));

  // Overlong encoding of '/'
  EXPECT_FALSE(is_valid_utf8("\xC0\xAF"));
}

TEST(Utf8UtilsTest, InvalidUtf8Surrogates)
{
  // UTF-16 surrogate pairs are invalid in UTF-8
  // U+D800 encoded as UTF-8
  EXPECT_FALSE(is_valid_utf8("\xED\xA0\x80"));
}

TEST(Utf8UtilsTest, InvalidUtf8InvalidLeadingByte)
{
  // Invalid leading bytes
  EXPECT_FALSE(is_valid_utf8("\xFE"));
  EXPECT_FALSE(is_valid_utf8("\xFF"));
}

// ============================================================================
// sanitize_utf8 Tests
// ============================================================================

TEST(Utf8UtilsTest, SanitizeValidUtf8)
{
  EXPECT_EQ(sanitize_utf8(ASCII_STR), ASCII_STR);
  EXPECT_EQ(sanitize_utf8(MIXED_STR), MIXED_STR);
  EXPECT_EQ(sanitize_utf8(THREE_BYTE_STR), THREE_BYTE_STR);
}

TEST(Utf8UtilsTest, SanitizeOrphanContinuation)
{
  EXPECT_EQ(sanitize_utf8("Hello\x80World"), "Hello?World");
  EXPECT_EQ(sanitize_utf8("\x80"), "?");
}

TEST(Utf8UtilsTest, SanitizeTruncatedSequence)
{
  // Truncated at end
  EXPECT_EQ(sanitize_utf8("Hi\xE4\xB8"), "Hi??");
}

TEST(Utf8UtilsTest, SanitizeOverlongEncoding)
{
  EXPECT_EQ(sanitize_utf8("\xC0\x80"), "??");
}

TEST(Utf8UtilsTest, SanitizeCustomReplacement)
{
  EXPECT_EQ(sanitize_utf8("Hello\x80World", '#'), "Hello#World");
  EXPECT_EQ(sanitize_utf8("\x80\x81\x82", 'X'), "XXX");
}

TEST(Utf8UtilsTest, SanitizeEmpty)
{
  EXPECT_EQ(sanitize_utf8(""), "");
}

TEST(Utf8UtilsTest, SanitizeAllInvalid)
{
  EXPECT_EQ(sanitize_utf8("\x80\x81\x82"), "???");
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(Utf8UtilsTest, BoundaryCodepoints)
{
  // U+007F - Last ASCII (1 byte)
  EXPECT_TRUE(is_valid_utf8("\x7F"));

  // U+0080 - First 2-byte (C2 80)
  EXPECT_TRUE(is_valid_utf8("\xC2\x80"));

  // U+07FF - Last 2-byte (DF BF)
  EXPECT_TRUE(is_valid_utf8("\xDF\xBF"));

  // U+0800 - First 3-byte (E0 A0 80)
  EXPECT_TRUE(is_valid_utf8("\xE0\xA0\x80"));

  // U+FFFF - Last 3-byte (EF BF BF)
  EXPECT_TRUE(is_valid_utf8("\xEF\xBF\xBF"));

  // U+10000 - First 4-byte (F0 90 80 80)
  EXPECT_TRUE(is_valid_utf8("\xF0\x90\x80\x80"));

  // U+10FFFF - Last valid (F4 8F BF BF)
  EXPECT_TRUE(is_valid_utf8("\xF4\x8F\xBF\xBF"));
}

TEST(Utf8UtilsTest, BeyondUnicode)
{
  // U+110000 - Beyond Unicode range (F4 90 80 80)
  EXPECT_FALSE(is_valid_utf8("\xF4\x90\x80\x80"));
}

} // namespace fb::test
