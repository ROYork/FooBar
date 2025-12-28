#include <fb/string_list.h>
#include <fb/string_utils.h>

#include <gtest/gtest.h>

#include <algorithm>

namespace fb
{

// ============================================================================
// Construction Tests
// ============================================================================

TEST(StringList, DefaultConstruction)
{
  string_list list;
  EXPECT_TRUE(list.empty());
  EXPECT_EQ(list.size(), 0u);
}

TEST(StringList, InitializerListConstruction)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "c");
}

TEST(StringList, VectorConstruction)
{
  std::vector<std::string> vec{"hello", "world"};
  string_list list(vec);
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list[0], "hello");
}

TEST(StringList, VectorMoveConstruction)
{
  std::vector<std::string> vec{"hello", "world"};
  string_list list(std::move(vec));
  EXPECT_EQ(list.size(), 2u);
}

TEST(StringList, CopyConstruction)
{
  string_list list1{"a", "b"};
  string_list list2(list1);
  EXPECT_EQ(list2, list1);
}

TEST(StringList, MoveConstruction)
{
  string_list list1{"a", "b"};
  string_list list2(std::move(list1));
  EXPECT_EQ(list2.size(), 2u);
}

// ============================================================================
// Factory Method Tests
// ============================================================================

TEST(StringList, FromSplit_Char)
{
  auto list = string_list::from_split("a,b,c", ',');
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "c");
}

TEST(StringList, FromSplit_String)
{
  auto list = string_list::from_split("a::b::c", "::");
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "c");
}

TEST(StringList, FromSplit_SkipEmpty)
{
  auto list = string_list::from_split("a,,b", ',', false);
  EXPECT_EQ(list.size(), 2u);
}

TEST(StringList, FromLines)
{
  auto list = string_list::from_lines("line1\nline2\nline3");
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "line1");
}

// ============================================================================
// Element Access Tests
// ============================================================================

TEST(StringList, At_Valid)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list.at(0), "a");
  EXPECT_EQ(list.at(2), "c");
}

TEST(StringList, At_OutOfRange)
{
  string_list list{"a", "b"};
  EXPECT_THROW(list.at(10), std::out_of_range);
}

TEST(StringList, OperatorBracket)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list[0], "a");
  list[0] = "x";
  EXPECT_EQ(list[0], "x");
}

TEST(StringList, FrontBack)
{
  string_list list{"first", "middle", "last"};
  EXPECT_EQ(list.front(), "first");
  EXPECT_EQ(list.back(), "last");
}

// ============================================================================
// Iterator Tests
// ============================================================================

TEST(StringList, RangeBasedFor)
{
  string_list list{"a", "b", "c"};
  std::string result;
  for (const auto& s : list)
  {
    result += s;
  }
  EXPECT_EQ(result, "abc");
}

TEST(StringList, Iterators)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(*list.begin(), "a");
  EXPECT_EQ(*(list.end() - 1), "c");
}

TEST(StringList, ReverseIterators)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(*list.rbegin(), "c");
}

// ============================================================================
// Capacity Tests
// ============================================================================

TEST(StringList, Empty)
{
  string_list empty_list;
  string_list non_empty{"a"};
  EXPECT_TRUE(empty_list.empty());
  EXPECT_FALSE(non_empty.empty());
}

TEST(StringList, Size)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list.size(), 3u);
}

TEST(StringList, Reserve)
{
  string_list list;
  list.reserve(100);
  EXPECT_GE(list.capacity(), 100u);
}

// ============================================================================
// Modifier Tests
// ============================================================================

TEST(StringList, Clear)
{
  string_list list{"a", "b", "c"};
  list.clear();
  EXPECT_TRUE(list.empty());
}

TEST(StringList, Insert)
{
  string_list list{"a", "c"};
  list.insert(list.begin() + 1, "b");
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[1], "b");
}

TEST(StringList, Erase)
{
  string_list list{"a", "b", "c"};
  list.erase(list.begin() + 1);
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list[1], "c");
}

TEST(StringList, PushBack)
{
  string_list list;
  list.push_back("a");
  list.push_back("b");
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list.back(), "b");
}

TEST(StringList, PopBack)
{
  string_list list{"a", "b", "c"};
  list.pop_back();
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list.back(), "b");
}

TEST(StringList, EmplaceBack)
{
  string_list list;
  list.emplace_back("hello");
  EXPECT_EQ(list.size(), 1u);
  EXPECT_EQ(list[0], "hello");
}

TEST(StringList, Resize)
{
  string_list list{"a", "b"};
  list.resize(5, "x");
  EXPECT_EQ(list.size(), 5u);
  EXPECT_EQ(list[2], "x");
}

TEST(StringList, Swap)
{
  string_list list1{"a", "b"};
  string_list list2{"x", "y", "z"};
  list1.swap(list2);
  EXPECT_EQ(list1.size(), 3u);
  EXPECT_EQ(list2.size(), 2u);
}

// ============================================================================
// String List Operations Tests
// ============================================================================

TEST(StringList, Join_String)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list.join(", "), "a, b, c");
  EXPECT_EQ(list.join(""), "abc");
}

TEST(StringList, Join_Char)
{
  string_list list{"a", "b", "c"};
  EXPECT_EQ(list.join(','), "a,b,c");
}

TEST(StringList, Join_Empty)
{
  string_list empty;
  EXPECT_EQ(empty.join(","), "");
}

TEST(StringList, Filter)
{
  string_list list{"apple", "banana", "apricot", "cherry"};
  auto filtered = list.filter([](const std::string& s) {
    return starts_with(s, "a");
  });
  EXPECT_EQ(filtered.size(), 2u);
  EXPECT_EQ(filtered[0], "apple");
  EXPECT_EQ(filtered[1], "apricot");
}

TEST(StringList, Map)
{
  string_list list{"a", "b", "c"};
  auto mapped = list.map([](const std::string& s) {
    return to_upper(s);
  });
  EXPECT_EQ(mapped[0], "A");
  EXPECT_EQ(mapped[1], "B");
  EXPECT_EQ(mapped[2], "C");
}

TEST(StringList, FilterContaining)
{
  string_list list{"hello", "world", "help", "wild"};
  auto filtered = list.filter_containing("el");
  EXPECT_EQ(filtered.size(), 2u);  // "hello" and "help" both contain "el"
}

TEST(StringList, FilterStartsWith)
{
  string_list list{"hello", "world", "help", "wild"};
  auto filtered = list.filter_starts_with("hel");
  EXPECT_EQ(filtered.size(), 2u);
}

TEST(StringList, FilterEndsWith)
{
  string_list list{"hello", "world", "jello", "wild"};
  auto filtered = list.filter_ends_with("llo");
  EXPECT_EQ(filtered.size(), 2u);
}

TEST(StringList, FilterMatching)
{
  string_list list{"abc123", "def456", "ghi789", "abc"};
  auto filtered = list.filter_matching("^abc");
  EXPECT_EQ(filtered.size(), 2u);
}

// ============================================================================
// Search Operation Tests
// ============================================================================

TEST(StringList, Contains)
{
  string_list list{"hello", "world"};
  EXPECT_TRUE(list.contains("hello"));
  EXPECT_FALSE(list.contains("hi"));
}

TEST(StringList, ContainsIgnoreCase)
{
  string_list list{"Hello", "World"};
  EXPECT_TRUE(list.contains_ignore_case("hello"));
  EXPECT_TRUE(list.contains_ignore_case("WORLD"));
  EXPECT_FALSE(list.contains_ignore_case("hi"));
}

TEST(StringList, IndexOf)
{
  string_list list{"a", "b", "c", "b"};
  EXPECT_EQ(list.index_of("b"), 1u);
  EXPECT_EQ(list.index_of("x"), std::nullopt);
}

TEST(StringList, LastIndexOf)
{
  string_list list{"a", "b", "c", "b"};
  EXPECT_EQ(list.last_index_of("b"), 3u);
  EXPECT_EQ(list.last_index_of("x"), std::nullopt);
}

TEST(StringList, Count)
{
  string_list list{"a", "b", "a", "c", "a"};
  EXPECT_EQ(list.count("a"), 3u);
  EXPECT_EQ(list.count("x"), 0u);
}

// ============================================================================
// Sorting and Ordering Tests
// ============================================================================

TEST(StringList, Sort)
{
  string_list list{"c", "a", "b"};
  list.sort();
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "c");
}

TEST(StringList, SortWithComparator)
{
  string_list list{"a", "b", "c"};
  list.sort([](const std::string& a, const std::string& b) {
    return a > b;
  });
  EXPECT_EQ(list[0], "c");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "a");
}

TEST(StringList, SortIgnoreCase)
{
  string_list list{"Banana", "apple", "Cherry"};
  list.sort_ignore_case();
  EXPECT_EQ(list[0], "apple");
  EXPECT_EQ(list[1], "Banana");
  EXPECT_EQ(list[2], "Cherry");
}

TEST(StringList, SortNatural)
{
  string_list list{"file10", "file2", "file1"};
  list.sort_natural();
  EXPECT_EQ(list[0], "file1");
  EXPECT_EQ(list[1], "file2");
  EXPECT_EQ(list[2], "file10");
}

TEST(StringList, Reverse)
{
  string_list list{"a", "b", "c"};
  list.reverse();
  EXPECT_EQ(list[0], "c");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "a");
}

TEST(StringList, Unique)
{
  string_list list{"a", "a", "b", "b", "c"};
  list.unique();
  EXPECT_EQ(list.size(), 3u);
}

// ============================================================================
// Utility Operation Tests
// ============================================================================

TEST(StringList, RemoveEmpty)
{
  string_list list{"a", "", "b", "", "c"};
  list.remove_empty();
  EXPECT_EQ(list.size(), 3u);
  EXPECT_FALSE(list.contains(""));
}

TEST(StringList, RemoveBlank)
{
  string_list list{"a", "   ", "b", "\t", "c"};
  list.remove_blank();
  EXPECT_EQ(list.size(), 3u);
}

TEST(StringList, TrimAll)
{
  string_list list{"  a  ", "  b  "};
  list.trim_all();
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
}

TEST(StringList, ToLowerAll)
{
  string_list list{"Hello", "WORLD"};
  list.to_lower_all();
  EXPECT_EQ(list[0], "hello");
  EXPECT_EQ(list[1], "world");
}

TEST(StringList, ToUpperAll)
{
  string_list list{"hello", "world"};
  list.to_upper_all();
  EXPECT_EQ(list[0], "HELLO");
  EXPECT_EQ(list[1], "WORLD");
}

TEST(StringList, RemoveDuplicates)
{
  string_list list{"a", "b", "a", "c", "b"};
  list.remove_duplicates();
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "a");
  EXPECT_EQ(list[1], "b");
  EXPECT_EQ(list[2], "c");
}

TEST(StringList, Take)
{
  string_list list{"a", "b", "c", "d"};
  auto taken = list.take(2);
  EXPECT_EQ(taken.size(), 2u);
  EXPECT_EQ(taken[0], "a");
  EXPECT_EQ(taken[1], "b");
}

TEST(StringList, Skip)
{
  string_list list{"a", "b", "c", "d"};
  auto skipped = list.skip(2);
  EXPECT_EQ(skipped.size(), 2u);
  EXPECT_EQ(skipped[0], "c");
  EXPECT_EQ(skipped[1], "d");
}

TEST(StringList, TakeLast)
{
  string_list list{"a", "b", "c", "d"};
  auto taken = list.take_last(2);
  EXPECT_EQ(taken.size(), 2u);
  EXPECT_EQ(taken[0], "c");
  EXPECT_EQ(taken[1], "d");
}

TEST(StringList, Slice)
{
  string_list list{"a", "b", "c", "d", "e"};
  auto sliced = list.slice(1, 4);
  EXPECT_EQ(sliced.size(), 3u);
  EXPECT_EQ(sliced[0], "b");
  EXPECT_EQ(sliced[1], "c");
  EXPECT_EQ(sliced[2], "d");
}

// ============================================================================
// Conversion Tests
// ============================================================================

TEST(StringList, ToVector)
{
  string_list list{"a", "b", "c"};
  std::vector<std::string>& vec = list.to_vector();
  EXPECT_EQ(vec.size(), 3u);
  vec[0] = "x";
  EXPECT_EQ(list[0], "x");
}

TEST(StringList, ToVectorCopy)
{
  string_list list{"a", "b", "c"};
  std::vector<std::string> copy = list.to_vector_copy();
  copy[0] = "x";
  EXPECT_EQ(list[0], "a"); // Original unchanged
}

// ============================================================================
// Operator Tests
// ============================================================================

TEST(StringList, Equality)
{
  string_list list1{"a", "b"};
  string_list list2{"a", "b"};
  string_list list3{"a", "c"};
  EXPECT_EQ(list1, list2);
  EXPECT_NE(list1, list3);
}

TEST(StringList, Concatenation)
{
  string_list list1{"a", "b"};
  string_list list2{"c", "d"};
  auto combined = list1 + list2;
  EXPECT_EQ(combined.size(), 4u);
  EXPECT_EQ(combined[0], "a");
  EXPECT_EQ(combined[3], "d");
}

TEST(StringList, ConcatAssign)
{
  string_list list{"a", "b"};
  list += string_list{"c", "d"};
  EXPECT_EQ(list.size(), 4u);
}

TEST(StringList, AppendString)
{
  string_list list{"a"};
  list += "b";
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list[1], "b");
}

TEST(StringList, StreamOperator)
{
  string_list list;
  list << "a" << "b" << "c";
  EXPECT_EQ(list.size(), 3u);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(StringList, LargeList)
{
  string_list list;
  for (int i = 0; i < 10000; ++i)
  {
    list.push_back(std::to_string(i));
  }
  EXPECT_EQ(list.size(), 10000u);
}

TEST(StringList, MethodChaining)
{
  string_list list{"  Hello  ", "  WORLD  ", "", "  foo  "};
  list.trim_all().remove_empty().to_lower_all().sort();
  EXPECT_EQ(list.size(), 3u);
  EXPECT_EQ(list[0], "foo");
  EXPECT_EQ(list[1], "hello");
  EXPECT_EQ(list[2], "world");
}

} // namespace fb
