#include <gtest/gtest.h>

#include "fb/circular_buffer.h"

#include <stdexcept>
#include <vector>

using fb::circular_buffer;

TEST(CircularBufferTest, PushPopMaintainsOrder) {
  circular_buffer<int> buffer(4);

  buffer.push_back(1);
  buffer.push_back(2);
  buffer.push_back(3);

  EXPECT_EQ(buffer.size(), 3u);
  EXPECT_EQ(buffer.front(), 1);
  EXPECT_EQ(buffer.back(), 3);

  buffer.pop_front();
  EXPECT_EQ(buffer.front(), 2);
  EXPECT_EQ(buffer.size(), 2u);
}

TEST(CircularBufferTest, WrapAroundPreservesSequence) {
  circular_buffer<int> buffer(3);
  buffer.push_back(1);
  buffer.push_back(2);
  buffer.push_back(3);

  buffer.pop_front();  // make room to wrap
  buffer.push_back(4); // wraps tail to index 0

  std::vector<int> values;
  for (int value : buffer) {
    values.push_back(value);
  }

  ASSERT_EQ(values, (std::vector<int>{2, 3, 4}));
}

TEST(CircularBufferTest, RejectsWhenFullIfPolicyDisabled) {
  circular_buffer<int, false> buffer(2);
  buffer.push_back(10);
  buffer.push_back(20);

  buffer.push_back(30);  // should be ignored when policy is disabled

  EXPECT_EQ(buffer.size(), 2u);
  EXPECT_EQ(buffer.front(), 10);
  EXPECT_EQ(buffer.back(), 20);
}

TEST(CircularBufferTest, OverwritesOldestWhenPolicyEnabled) {
  circular_buffer<int, true> buffer(2);
  buffer.push_back(1);
  buffer.push_back(2);

  buffer.push_back(3);  // should evict the oldest element

  EXPECT_EQ(buffer.size(), 2u);
  EXPECT_EQ(buffer.front(), 2);
  EXPECT_EQ(buffer.back(), 3);
}

TEST(CircularBufferTest, SupportsSingleSlotBuffers) {
  circular_buffer<int> buffer(1);
  buffer.push_back(7);
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(buffer.front(), 7);

  buffer.pop_front();
  EXPECT_TRUE(buffer.empty());

  buffer.push_back(9);
  EXPECT_EQ(buffer.front(), 9);
}

TEST(CircularBufferTest, AtThrowsWhenIndexOutOfRange) {
  circular_buffer<int> buffer(2);
  buffer.push_back(42);
  EXPECT_NO_THROW(buffer.at(0));
  EXPECT_THROW(buffer.at(1), std::out_of_range);
}

TEST(CircularBufferTest, ReserveKeepsContents) {
  circular_buffer<int> buffer(2);
  buffer.push_back(5);
  buffer.push_back(6);

  buffer.reserve(4);
  EXPECT_GE(buffer.capacity(), 4u);
  ASSERT_EQ(buffer.size(), 2u);
  EXPECT_EQ(buffer.front(), 5);
  EXPECT_EQ(buffer.back(), 6);

  buffer.push_back(7);
  EXPECT_EQ(buffer.back(), 7);
}

TEST(CircularBufferTest, IteratorSupportsStdAlgorithms) {
  circular_buffer<int> buffer(5);
  for (int i = 0; i < 5; ++i) {
    buffer.push_back(i);
  }

  const std::vector<int> expected{0, 1, 2, 3, 4};
  std::vector<int> actual(buffer.begin(), buffer.end());
  EXPECT_EQ(actual, expected);

  std::vector<int> reversed(buffer.rbegin(), buffer.rend());
  EXPECT_EQ(reversed, (std::vector<int>{4, 3, 2, 1, 0}));
}

TEST(CircularBufferTest, EqualityOperatorsCompareContents) {
  circular_buffer<int> a(3);
  circular_buffer<int> b(3);

  a.push_back(1);
  a.push_back(2);
  b.push_back(1);
  b.push_back(2);

  EXPECT_TRUE(a == b);
  b.push_back(3);
  EXPECT_TRUE(a != b);
  a.push_back(3);
  EXPECT_FALSE(a < b);
}
