/// @file test_string_hash.cpp
/// @brief Unit tests for string hash functions

#include <gtest/gtest.h>

#include "fb/string_hash.h"

#include <set>
#include <string>
#include <vector>

namespace fb::test
{

// ============================================================================
// FNV-1a 32-bit Tests
// ============================================================================

TEST(StringHashTest, Fnv1a32EmptyString)
{
  // FNV-1a offset basis for empty string
  EXPECT_EQ(hash_fnv1a_32(""), 0x811c9dc5);
}

TEST(StringHashTest, Fnv1a32KnownValues)
{
  // Known FNV-1a test vectors
  EXPECT_EQ(hash_fnv1a_32("a"), 0xe40c292cU);
  EXPECT_EQ(hash_fnv1a_32("foobar"), 0xbf9cf968U);
}

TEST(StringHashTest, Fnv1a32Deterministic)
{
  std::string test_string = "Hello, World!";
  uint32_t    h1          = hash_fnv1a_32(test_string);
  uint32_t    h2          = hash_fnv1a_32(test_string);
  EXPECT_EQ(h1, h2);
}

TEST(StringHashTest, Fnv1a32DifferentStrings)
{
  uint32_t h1 = hash_fnv1a_32("hello");
  uint32_t h2 = hash_fnv1a_32("world");
  EXPECT_NE(h1, h2);
}

TEST(StringHashTest, Fnv1a32CaseSensitive)
{
  uint32_t h1 = hash_fnv1a_32("Hello");
  uint32_t h2 = hash_fnv1a_32("hello");
  EXPECT_NE(h1, h2);
}

TEST(StringHashTest, Fnv1a32Distribution)
{
  // Test that hash distributes well across similar strings
  std::set<uint32_t> hashes;
  for (int i = 0; i < 1000; ++i)
  {
    std::string s = "test_" + std::to_string(i);
    hashes.insert(hash_fnv1a_32(s));
  }
  // Should have very few collisions
  EXPECT_GE(hashes.size(), 990);
}

// ============================================================================
// FNV-1a 64-bit Tests
// ============================================================================

TEST(StringHashTest, Fnv1a64EmptyString)
{
  // FNV-1a 64-bit offset basis
  EXPECT_EQ(hash_fnv1a_64(""), 0xcbf29ce484222325ULL);
}

TEST(StringHashTest, Fnv1a64KnownValues)
{
  // Known FNV-1a 64-bit test vectors
  EXPECT_EQ(hash_fnv1a_64("a"), 0xaf63dc4c8601ec8cULL);
  EXPECT_EQ(hash_fnv1a_64("foobar"), 0x85944171f73967e8ULL);
}

TEST(StringHashTest, Fnv1a64Deterministic)
{
  std::string test_string = "Hello, World!";
  uint64_t    h1          = hash_fnv1a_64(test_string);
  uint64_t    h2          = hash_fnv1a_64(test_string);
  EXPECT_EQ(h1, h2);
}

TEST(StringHashTest, Fnv1a64DifferentStrings)
{
  uint64_t h1 = hash_fnv1a_64("hello");
  uint64_t h2 = hash_fnv1a_64("world");
  EXPECT_NE(h1, h2);
}

TEST(StringHashTest, Fnv1a64Distribution)
{
  std::set<uint64_t> hashes;
  for (int i = 0; i < 10000; ++i)
  {
    std::string s = "test_" + std::to_string(i);
    hashes.insert(hash_fnv1a_64(s));
  }
  // 64-bit should have no collisions for 10k strings
  EXPECT_EQ(hashes.size(), 10000);
}

// ============================================================================
// DJB2 Tests
// ============================================================================

TEST(StringHashTest, Djb2EmptyString)
{
  // DJB2 returns 5381 for empty string
  EXPECT_EQ(hash_djb2(""), 5381);
}

TEST(StringHashTest, Djb2KnownValue)
{
  // DJB2 test value - verify determinism
  EXPECT_EQ(hash_djb2("hello"), 261238937U);
}

TEST(StringHashTest, Djb2Deterministic)
{
  std::string test_string = "Test String";
  uint32_t    h1          = hash_djb2(test_string);
  uint32_t    h2          = hash_djb2(test_string);
  EXPECT_EQ(h1, h2);
}

TEST(StringHashTest, Djb2DifferentStrings)
{
  uint32_t h1 = hash_djb2("hello");
  uint32_t h2 = hash_djb2("world");
  EXPECT_NE(h1, h2);
}

TEST(StringHashTest, Djb2Distribution)
{
  std::set<uint32_t> hashes;
  for (int i = 0; i < 1000; ++i)
  {
    std::string s = "item_" + std::to_string(i);
    hashes.insert(hash_djb2(s));
  }
  EXPECT_GE(hashes.size(), 990);
}

// ============================================================================
// Case-Insensitive Hash Tests
// ============================================================================

TEST(StringHashTest, CaseInsensitiveEmptyString)
{
  EXPECT_EQ(hash_case_insensitive(""), 0x811c9dc5);
}

TEST(StringHashTest, CaseInsensitiveSameForDifferentCase)
{
  uint32_t h1 = hash_case_insensitive("Hello");
  uint32_t h2 = hash_case_insensitive("HELLO");
  uint32_t h3 = hash_case_insensitive("hello");
  uint32_t h4 = hash_case_insensitive("HeLLo");

  EXPECT_EQ(h1, h2);
  EXPECT_EQ(h2, h3);
  EXPECT_EQ(h3, h4);
}

TEST(StringHashTest, CaseInsensitiveDifferentStrings)
{
  uint32_t h1 = hash_case_insensitive("hello");
  uint32_t h2 = hash_case_insensitive("world");
  EXPECT_NE(h1, h2);
}

TEST(StringHashTest, CaseInsensitiveWithNumbers)
{
  uint32_t h1 = hash_case_insensitive("Test123");
  uint32_t h2 = hash_case_insensitive("TEST123");
  EXPECT_EQ(h1, h2);
}

TEST(StringHashTest, CaseInsensitiveWithSymbols)
{
  uint32_t h1 = hash_case_insensitive("Hello, World!");
  uint32_t h2 = hash_case_insensitive("HELLO, WORLD!");
  EXPECT_EQ(h1, h2);
}

TEST(StringHashTest, CaseInsensitiveDeterministic)
{
  std::string test_string = "CaSe TeSt";
  uint32_t    h1          = hash_case_insensitive(test_string);
  uint32_t    h2          = hash_case_insensitive(test_string);
  EXPECT_EQ(h1, h2);
}

// ============================================================================
// CRC-32 Tests
// ============================================================================

TEST(StringHashTest, Crc32EmptyString)
{
  // CRC-32 of empty string is 0
  EXPECT_EQ(crc32(""), 0x00000000);
}

TEST(StringHashTest, Crc32KnownValues)
{
  // Known CRC-32 test vectors (IEEE 802.3 polynomial)
  EXPECT_EQ(crc32("123456789"), 0xCBF43926);
}

TEST(StringHashTest, Crc32SingleChar)
{
  // CRC-32 of "a"
  EXPECT_EQ(crc32("a"), 0xe8b7be43);
}

TEST(StringHashTest, Crc32Deterministic)
{
  std::string test_data = "Test data for CRC";
  uint32_t    c1        = crc32(test_data);
  uint32_t    c2        = crc32(test_data);
  EXPECT_EQ(c1, c2);
}

TEST(StringHashTest, Crc32DifferentStrings)
{
  uint32_t c1 = crc32("hello");
  uint32_t c2 = crc32("world");
  EXPECT_NE(c1, c2);
}

TEST(StringHashTest, Crc32BinaryData)
{
  std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0x03, 0x04};
  uint32_t             c    = crc32(data.data(), data.size());
  EXPECT_EQ(c, 1364906956U);
}

TEST(StringHashTest, Crc32WithNullBytes)
{
  std::string with_null = std::string("hello\0world", 11);
  uint32_t    c         = crc32(with_null);
  // Should hash all 11 bytes including the null
  EXPECT_NE(c, crc32("hello"));
  EXPECT_NE(c, crc32("helloworld"));
}

// ============================================================================
// General Hash Property Tests
// ============================================================================

TEST(StringHashTest, AllHashesDiffer)
{
  // All hash functions should produce different values for same input
  std::string test = "TestString";

  uint32_t fnv32         = hash_fnv1a_32(test);
  uint64_t fnv64         = hash_fnv1a_64(test);
  uint32_t djb2          = hash_djb2(test);
  uint32_t case_insens   = hash_case_insensitive(test);
  uint32_t crc           = crc32(test);
  uint32_t case_insens_l = hash_case_insensitive("teststring");

  // FNV-32 and case-insensitive with lowercase should match
  EXPECT_EQ(case_insens, case_insens_l);

  // All others should differ (very unlikely to be same)
  EXPECT_NE(fnv32, djb2);
  EXPECT_NE(fnv32, crc);
  EXPECT_NE(djb2, crc);
  EXPECT_NE(static_cast<uint32_t>(fnv64), fnv32); // Different algorithms
}

TEST(StringHashTest, LongStrings)
{
  std::string long_string(10000, 'x');

  // All hash functions should handle long strings
  uint32_t fnv32 = hash_fnv1a_32(long_string);
  uint64_t fnv64 = hash_fnv1a_64(long_string);
  uint32_t djb2  = hash_djb2(long_string);
  uint32_t ci    = hash_case_insensitive(long_string);
  uint32_t c     = crc32(long_string);

  // Just ensure they complete and produce values
  EXPECT_NE(fnv32, 0);
  EXPECT_NE(fnv64, 0);
  EXPECT_NE(djb2, 0);
  EXPECT_NE(ci, 0);
  // CRC could legitimately be 0 but very unlikely for this input
  (void)c;
}

TEST(StringHashTest, SpecialCharacters)
{
  std::string special = "\t\n\r\x00\x1F\x7F";

  // All hash functions should handle special characters - verify they produce values
  EXPECT_NE(hash_fnv1a_32(special), 0U);
  EXPECT_NE(hash_fnv1a_64(special), 0ULL);
  EXPECT_NE(hash_djb2(special), 5381U); // 5381 is empty string hash
  EXPECT_NE(hash_case_insensitive(special), 0x811c9dc5U); // Empty string hash
  // CRC could be anything, just verify it runs
  (void)crc32(special);
}

} // namespace fb::test
