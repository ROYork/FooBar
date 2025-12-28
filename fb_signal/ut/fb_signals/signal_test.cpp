/// @file signal_test.cpp
/// @brief Unit tests for signal emission and basic operations

#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <vector>

namespace fb {
namespace test {

// ============================================================================
// Basic Signal Tests
// ============================================================================

TEST(SignalTest, EmptySignal_NoSlots_EmitsWithoutError) {
  signal<int> sig;

  EXPECT_TRUE(sig.empty());
  EXPECT_EQ(0u, sig.slot_count());

  // Should not crash
  sig.emit(42);
}

TEST(SignalTest, Connect_SingleSlot_InvokesOnEmit) {
  signal<int> sig;
  int received = 0;

  sig.connect([&received](int value) { received = value; });

  EXPECT_EQ(1u, sig.slot_count());
  EXPECT_FALSE(sig.empty());

  sig.emit(42);

  EXPECT_EQ(42, received);
}

TEST(SignalTest, Connect_MultipleSlots_AllInvoked) {
  signal<int> sig;
  std::vector<int> received;

  sig.connect([&received](int value) { received.push_back(value * 1); });
  sig.connect([&received](int value) { received.push_back(value * 2); });
  sig.connect([&received](int value) { received.push_back(value * 3); });

  EXPECT_EQ(3u, sig.slot_count());

  sig.emit(10);

  ASSERT_EQ(3u, received.size());
  // Note: order depends on priority (all same = connection order)
  EXPECT_EQ(10, received[0]);
  EXPECT_EQ(20, received[1]);
  EXPECT_EQ(30, received[2]);
}

TEST(SignalTest, Emit_MultipleArguments_AllForwarded) {
  signal<int, const std::string &, double> sig;

  int received_int = 0;
  std::string received_str;
  double received_double = 0.0;

  sig.connect([&](int i, const std::string &s, double d) {
    received_int = i;
    received_str = s;
    received_double = d;
  });

  sig.emit(42, "hello", 3.14);

  EXPECT_EQ(42, received_int);
  EXPECT_EQ("hello", received_str);
  EXPECT_DOUBLE_EQ(3.14, received_double);
}

TEST(SignalTest, Emit_VoidSignal_Works) {
  signal<> sig; // No arguments
  int call_count = 0;

  sig.connect([&call_count]() { ++call_count; });

  sig.emit();
  sig.emit();
  sig.emit();

  EXPECT_EQ(3, call_count);
}

TEST(SignalTest, Emit_FunctionCallOperator_Works) {
  signal<int> sig;
  int received = 0;

  sig.connect([&received](int value) { received = value; });

  sig(100); // Use operator() instead of emit()

  EXPECT_EQ(100, received);
}

// ============================================================================
// Priority Tests
// ============================================================================

TEST(SignalTest, Priority_HigherPriorityFirst) {
  signal<int> sig;
  std::vector<int> order;

  sig.connect([&order](int) { order.push_back(1); }, priority::low);
  sig.connect([&order](int) { order.push_back(2); }, priority::high);
  sig.connect([&order](int) { order.push_back(3); }, priority::normal);

  sig.emit(0);

  ASSERT_EQ(3u, order.size());
  EXPECT_EQ(2, order[0]); // high first
  EXPECT_EQ(3, order[1]); // normal second
  EXPECT_EQ(1, order[2]); // low last
}

TEST(SignalTest, Priority_SamePriority_ConnectionOrder) {
  signal<int> sig;
  std::vector<int> order;

  sig.connect([&order](int) { order.push_back(1); }, priority::normal);
  sig.connect([&order](int) { order.push_back(2); }, priority::normal);
  sig.connect([&order](int) { order.push_back(3); }, priority::normal);

  sig.emit(0);

  ASSERT_EQ(3u, order.size());
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(2, order[1]);
  EXPECT_EQ(3, order[2]);
}

// ============================================================================
// Disconnect Tests
// ============================================================================

TEST(SignalTest, DisconnectAll_ClearsAllSlots) {
  signal<int> sig;
  int count = 0;

  sig.connect([&count](int) { ++count; });
  sig.connect([&count](int) { ++count; });
  sig.connect([&count](int) { ++count; });

  sig.emit(0);
  EXPECT_EQ(3, count);

  sig.disconnect_all();
  EXPECT_TRUE(sig.empty());

  sig.emit(0);
  EXPECT_EQ(3, count); // No additional calls
}

TEST(SignalTest, Cleanup_RemovesDeactivatedSlots) {
  signal<int> sig;

  auto conn1 = sig.connect([](int) {});
  auto conn2 = sig.connect([](int) {});
  auto conn3 = sig.connect([](int) {});

  EXPECT_EQ(3u, sig.slot_count());

  conn2.disconnect();

  // Slot is deactivated but not yet removed
  // (implementation detail: may still count until cleanup)

  sig.cleanup();

  EXPECT_EQ(2u, sig.slot_count());
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST(SignalTest, ConnectFiltered_FilterAccepts_SlotInvoked) {
  signal<int> sig;
  int received = 0;

  sig.connect_filtered([&received](int value) { received = value; },
                       [](int value) { return value > 10; });

  sig.emit(5);
  EXPECT_EQ(0, received);

  sig.emit(15);
  EXPECT_EQ(15, received);
}

TEST(SignalTest, ConnectFiltered_FilterRejects_SlotNotInvoked) {
  signal<int> sig;
  int call_count = 0;

  sig.connect_filtered(
      [&call_count](int) { ++call_count; },
      [](int value) { return value % 2 == 0; } // Only even numbers
  );

  sig.emit(1);
  sig.emit(3);
  sig.emit(5);
  EXPECT_EQ(0, call_count);

  sig.emit(2);
  sig.emit(4);
  EXPECT_EQ(2, call_count);
}

// ============================================================================
// Large Argument Tests
// ============================================================================

TEST(SignalTest, LargeArguments_CorrectlyForwarded) {
  signal<std::vector<int>> sig;
  std::size_t received_size = 0;

  sig.connect([&received_size](const std::vector<int> &vec) {
    received_size = vec.size();
  });

  std::vector<int> large_vec(1000, 42);
  sig.emit(large_vec);

  EXPECT_EQ(1000u, received_size);
}

TEST(SignalTest, MoveOnlyArguments_Work) {
  signal<std::unique_ptr<int>> sig;
  int received_value = 0;

  sig.connect([&received_value](std::unique_ptr<int> ptr) {
    if (ptr) {
      received_value = *ptr;
    }
  });

  sig.emit(std::make_unique<int>(42));

  EXPECT_EQ(42, received_value);
}

// ============================================================================
// Multi-Slot Rvalue Argument Tests
// These tests verify that multiple slots all receive the same value when
// emitting with rvalue arguments (temporary or std::move). This was a bug
// where std::forward in the emit loop caused the first slot to move-from
// the argument, leaving subsequent slots with moved-from (empty) data.
// ============================================================================

TEST(SignalTest, MultipleSlots_RvalueString_AllReceiveSameValue) {
  signal<std::string> sig;
  std::vector<std::string> received;

  // Connect multiple slots that take by value (triggers copy/move)
  sig.connect([&received](std::string s) { received.push_back(s); });
  sig.connect([&received](std::string s) { received.push_back(s); });
  sig.connect([&received](std::string s) { received.push_back(s); });

  // Emit with a temporary (rvalue)
  sig.emit(std::string("hello"));

  ASSERT_EQ(3u, received.size());
  EXPECT_EQ("hello", received[0]);
  EXPECT_EQ("hello", received[1]);
  EXPECT_EQ("hello", received[2]);
}

TEST(SignalTest, MultipleSlots_StdMoveString_AllReceiveSameValue) {
  signal<std::string> sig;
  std::vector<std::string> received;

  sig.connect([&received](std::string s) { received.push_back(s); });
  sig.connect([&received](std::string s) { received.push_back(s); });
  sig.connect([&received](std::string s) { received.push_back(s); });

  // Emit with std::move (rvalue)
  std::string msg = "world";
  sig.emit(std::move(msg));

  ASSERT_EQ(3u, received.size());
  EXPECT_EQ("world", received[0]);
  EXPECT_EQ("world", received[1]);
  EXPECT_EQ("world", received[2]);
}

TEST(SignalTest, MultipleSlots_MixedSignatures_AllReceiveSameValue) {
  signal<std::string> sig;
  std::vector<std::string> received;

  // Mix of slot signatures: by value, const ref, and const ref
  sig.connect([&received](std::string s) { received.push_back(s); });
  sig.connect([&received](const std::string& s) { received.push_back(s); });
  sig.connect([&received](const std::string& s) { received.push_back(s); });

  sig.emit(std::string("mixed"));

  ASSERT_EQ(3u, received.size());
  EXPECT_EQ("mixed", received[0]);
  EXPECT_EQ("mixed", received[1]);
  EXPECT_EQ("mixed", received[2]);
}

TEST(SignalTest, MultipleSlots_RvalueVector_AllReceiveSameValue) {
  signal<std::vector<int>> sig;
  std::vector<size_t> received_sizes;
  std::vector<int> received_first_elements;

  sig.connect([&](std::vector<int> v) {
    received_sizes.push_back(v.size());
    if (!v.empty()) received_first_elements.push_back(v[0]);
  });
  sig.connect([&](std::vector<int> v) {
    received_sizes.push_back(v.size());
    if (!v.empty()) received_first_elements.push_back(v[0]);
  });
  sig.connect([&](std::vector<int> v) {
    received_sizes.push_back(v.size());
    if (!v.empty()) received_first_elements.push_back(v[0]);
  });

  // Emit with a temporary vector (rvalue)
  sig.emit(std::vector<int>{1, 2, 3, 4, 5});

  ASSERT_EQ(3u, received_sizes.size());
  EXPECT_EQ(5u, received_sizes[0]);
  EXPECT_EQ(5u, received_sizes[1]);
  EXPECT_EQ(5u, received_sizes[2]);

  ASSERT_EQ(3u, received_first_elements.size());
  EXPECT_EQ(1, received_first_elements[0]);
  EXPECT_EQ(1, received_first_elements[1]);
  EXPECT_EQ(1, received_first_elements[2]);
}

TEST(SignalTest, MultipleSlots_MultipleRvalueArgs_AllReceiveSameValues) {
  signal<std::string, std::vector<int>> sig;
  std::vector<std::pair<std::string, size_t>> received;

  sig.connect([&](std::string s, std::vector<int> v) {
    received.emplace_back(s, v.size());
  });
  sig.connect([&](std::string s, std::vector<int> v) {
    received.emplace_back(s, v.size());
  });

  sig.emit(std::string("test"), std::vector<int>{1, 2, 3});

  ASSERT_EQ(2u, received.size());
  EXPECT_EQ("test", received[0].first);
  EXPECT_EQ(3u, received[0].second);
  EXPECT_EQ("test", received[1].first);
  EXPECT_EQ(3u, received[1].second);
}

TEST(SignalTest, SingleSlot_RvalueString_StillWorks) {
  // Ensure single-slot case still works efficiently
  signal<std::string> sig;
  std::string received;

  sig.connect([&received](std::string s) { received = std::move(s); });

  sig.emit(std::string("single"));

  EXPECT_EQ("single", received);
}

TEST(SignalTest, MultipleSlots_LargeString_AllReceiveFullContent) {
  signal<std::string> sig;
  std::vector<size_t> received_sizes;

  sig.connect([&](std::string s) { received_sizes.push_back(s.size()); });
  sig.connect([&](std::string s) { received_sizes.push_back(s.size()); });
  sig.connect([&](std::string s) { received_sizes.push_back(s.size()); });

  // Large string to ensure SSO doesn't mask the issue
  std::string large_string(10000, 'x');
  sig.emit(std::move(large_string));

  ASSERT_EQ(3u, received_sizes.size());
  EXPECT_EQ(10000u, received_sizes[0]);
  EXPECT_EQ(10000u, received_sizes[1]);
  EXPECT_EQ(10000u, received_sizes[2]);
}

} // namespace test
} // namespace fb
