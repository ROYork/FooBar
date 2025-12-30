/// @file test_string_random.cpp
/// @brief Unit tests for random string generation utilities

#include <gtest/gtest.h>

#include "fb/string_random.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <set>
#include <string>

namespace fb::test
{

// ============================================================================
// random_string Tests
// ============================================================================

TEST(StringRandomTest, RandomStringLength)
{
  EXPECT_EQ(random_string(0).size(), 0);
  EXPECT_EQ(random_string(1).size(), 1);
  EXPECT_EQ(random_string(10).size(), 10);
  EXPECT_EQ(random_string(100).size(), 100);
  EXPECT_EQ(random_string(1000).size(), 1000);
}

TEST(StringRandomTest, RandomStringCustomCharset)
{
  std::string result = random_string(100, "ABC");

  // All characters should be from the charset
  for (char c : result)
  {
    EXPECT_TRUE(c == 'A' || c == 'B' || c == 'C');
  }
}

TEST(StringRandomTest, RandomStringSingleCharCharset)
{
  std::string result = random_string(10, "X");
  EXPECT_EQ(result, "XXXXXXXXXX");
}

TEST(StringRandomTest, RandomStringEmptyCharsetThrows)
{
  EXPECT_THROW(random_string(10, ""), std::invalid_argument);
}

TEST(StringRandomTest, RandomStringZeroLength)
{
  std::string result = random_string(0, "ABC");
  EXPECT_TRUE(result.empty());
}

TEST(StringRandomTest, RandomStringRandomness)
{
  // Generate multiple strings and check they're different
  std::set<std::string> strings;
  for (int i = 0; i < 100; ++i)
  {
    strings.insert(random_string(20));
  }
  // Should be highly unlikely to have duplicates
  EXPECT_EQ(strings.size(), 100);
}

// ============================================================================
// random_alphanumeric Tests
// ============================================================================

TEST(StringRandomTest, AlphanumericLength)
{
  EXPECT_EQ(random_alphanumeric(0).size(), 0);
  EXPECT_EQ(random_alphanumeric(50).size(), 50);
}

TEST(StringRandomTest, AlphanumericCharset)
{
  std::string result = random_alphanumeric(1000);

  for (char c : result)
  {
    EXPECT_TRUE(std::isalnum(static_cast<unsigned char>(c)));
  }
}

TEST(StringRandomTest, AlphanumericDistribution)
{
  std::string result = random_alphanumeric(10000);

  // Should contain both letters and digits
  bool has_letter = std::any_of(result.begin(), result.end(), [](char c) {
    return std::isalpha(static_cast<unsigned char>(c));
  });

  bool has_digit = std::any_of(result.begin(), result.end(), [](char c) {
    return std::isdigit(static_cast<unsigned char>(c));
  });

  EXPECT_TRUE(has_letter);
  EXPECT_TRUE(has_digit);
}

// ============================================================================
// random_alphabetic Tests
// ============================================================================

TEST(StringRandomTest, AlphabeticLength)
{
  EXPECT_EQ(random_alphabetic(0).size(), 0);
  EXPECT_EQ(random_alphabetic(50).size(), 50);
}

TEST(StringRandomTest, AlphabeticCharset)
{
  std::string result = random_alphabetic(1000);

  for (char c : result)
  {
    EXPECT_TRUE(std::isalpha(static_cast<unsigned char>(c)));
  }
}

TEST(StringRandomTest, AlphabeticNoDigits)
{
  std::string result = random_alphabetic(1000);

  bool has_digit = std::any_of(result.begin(), result.end(), [](char c) {
    return std::isdigit(static_cast<unsigned char>(c));
  });

  EXPECT_FALSE(has_digit);
}

// ============================================================================
// random_numeric Tests
// ============================================================================

TEST(StringRandomTest, NumericLength)
{
  EXPECT_EQ(random_numeric(0).size(), 0);
  EXPECT_EQ(random_numeric(50).size(), 50);
}

TEST(StringRandomTest, NumericCharset)
{
  std::string result = random_numeric(1000);

  for (char c : result)
  {
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(c)));
  }
}

TEST(StringRandomTest, NumericNoLetters)
{
  std::string result = random_numeric(1000);

  bool has_letter = std::any_of(result.begin(), result.end(), [](char c) {
    return std::isalpha(static_cast<unsigned char>(c));
  });

  EXPECT_FALSE(has_letter);
}

// ============================================================================
// random_hex Tests
// ============================================================================

TEST(StringRandomTest, HexLength)
{
  EXPECT_EQ(random_hex(0).size(), 0);
  EXPECT_EQ(random_hex(50).size(), 50);
}

TEST(StringRandomTest, HexLowercaseCharset)
{
  std::string result = random_hex(1000, false);

  for (char c : result)
  {
    EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)));
    if (std::isalpha(static_cast<unsigned char>(c)))
    {
      EXPECT_TRUE(std::islower(static_cast<unsigned char>(c)));
    }
  }
}

TEST(StringRandomTest, HexUppercaseCharset)
{
  std::string result = random_hex(1000, true);

  for (char c : result)
  {
    EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c)));
    if (std::isalpha(static_cast<unsigned char>(c)))
    {
      EXPECT_TRUE(std::isupper(static_cast<unsigned char>(c)));
    }
  }
}

TEST(StringRandomTest, HexDefaultLowercase)
{
  std::string result = random_hex(100);

  bool has_uppercase = std::any_of(result.begin(), result.end(), [](char c) {
    return std::isupper(static_cast<unsigned char>(c));
  });

  EXPECT_FALSE(has_uppercase);
}

// ============================================================================
// random_uuid Tests
// ============================================================================

TEST(StringRandomTest, UuidLength)
{
  std::string uuid = random_uuid();
  EXPECT_EQ(uuid.size(), 36);
}

TEST(StringRandomTest, UuidFormat)
{
  std::string uuid = random_uuid();

  // Check format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  EXPECT_EQ(uuid[8], '-');
  EXPECT_EQ(uuid[13], '-');
  EXPECT_EQ(uuid[18], '-');
  EXPECT_EQ(uuid[23], '-');
}

TEST(StringRandomTest, UuidVersion4)
{
  std::string uuid = random_uuid();

  // Version 4: 13th character should be '4'
  EXPECT_EQ(uuid[14], '4');
}

TEST(StringRandomTest, UuidVariant)
{
  std::string uuid = random_uuid();

  // Variant: 17th character should be 8, 9, a, or b
  char variant = uuid[19];
  EXPECT_TRUE(variant == '8' || variant == '9' || variant == 'a' || variant == 'b');
}

TEST(StringRandomTest, UuidHexChars)
{
  std::string uuid = random_uuid();

  for (std::size_t i = 0; i < uuid.size(); ++i)
  {
    if (i == 8 || i == 13 || i == 18 || i == 23)
    {
      EXPECT_EQ(uuid[i], '-');
    }
    else
    {
      EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(uuid[i])));
    }
  }
}

TEST(StringRandomTest, UuidLowercase)
{
  std::string uuid = random_uuid();

  for (char c : uuid)
  {
    if (std::isalpha(static_cast<unsigned char>(c)))
    {
      EXPECT_TRUE(std::islower(static_cast<unsigned char>(c)));
    }
  }
}

TEST(StringRandomTest, UuidUniqueness)
{
  std::set<std::string> uuids;
  for (int i = 0; i < 1000; ++i)
  {
    uuids.insert(random_uuid());
  }
  // All UUIDs should be unique
  EXPECT_EQ(uuids.size(), 1000);
}

TEST(StringRandomTest, UuidRegexMatch)
{
  std::string uuid = random_uuid();

  // RFC-4122 UUID v4 pattern
  std::regex uuid_pattern(
      "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");

  EXPECT_TRUE(std::regex_match(uuid, uuid_pattern));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(StringRandomTest, MultipleCallsNoExceptions)
{
  // Just ensure rapid calls don't crash
  for (int i = 0; i < 10000; ++i)
  {
    EXPECT_NO_THROW(random_string(10));
    EXPECT_NO_THROW(random_alphanumeric(10));
    EXPECT_NO_THROW(random_alphabetic(10));
    EXPECT_NO_THROW(random_numeric(10));
    EXPECT_NO_THROW(random_hex(10));
    EXPECT_NO_THROW(random_uuid());
  }
}

} // namespace fb::test
