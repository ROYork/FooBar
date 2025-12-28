/// @file edge_cases_test.cpp
/// @brief Unit tests for reentrancy, self-disconnect, and other edge cases

#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace fb {
namespace test {

// ============================================================================
// Reentrancy Tests
// ============================================================================

TEST(EdgeCasesTest, ReentrantEmission_SameSignal) {
  signal<int> sig;
  std::vector<int> order;
  int depth = 0;

  sig.connect([&](int value) {
    order.push_back(value);

    if (depth < 3) {
      ++depth;
      sig.emit(value + 1); // Reentrant emit
      --depth;
    }
  });

  sig.emit(0);

  // Should see nested emissions
  ASSERT_EQ(4u, order.size());
  EXPECT_EQ(0, order[0]);
  EXPECT_EQ(1, order[1]);
  EXPECT_EQ(2, order[2]);
  EXPECT_EQ(3, order[3]);
}

TEST(EdgeCasesTest, ReentrantEmission_DifferentSignal) {
  signal<int> sig1;
  signal<int> sig2;
  std::vector<std::string> order;

  sig1.connect([&](int value) {
    order.push_back("sig1: " + std::to_string(value));
    sig2.emit(value * 10);
  });

  sig2.connect(
      [&](int value) { order.push_back("sig2: " + std::to_string(value)); });

  sig1.emit(5);

  ASSERT_EQ(2u, order.size());
  EXPECT_EQ("sig1: 5", order[0]);
  EXPECT_EQ("sig2: 50", order[1]);
}

// ============================================================================
// Self-Disconnect Tests
// ============================================================================

TEST(EdgeCasesTest, SelfDisconnect_DuringEmission) {
  signal<int> sig;
  int call_count = 0;
  connection conn;

  conn = sig.connect([&](int) {
    ++call_count;
    conn.disconnect(); // Disconnect self
  });

  sig.emit(0);
  EXPECT_EQ(1, call_count);

  sig.emit(0);
  EXPECT_EQ(1, call_count); // Not called again
}

TEST(EdgeCasesTest, SelfDisconnect_MultipleSlots) {
  signal<int> sig;
  std::vector<int> order;
  connection conn2;

  sig.connect([&](int) { order.push_back(1); });

  conn2 = sig.connect([&](int) {
    order.push_back(2);
    conn2.disconnect(); // Disconnect self
  });

  sig.connect([&](int) { order.push_back(3); });

  sig.emit(0);

  // All should fire in first emission
  ASSERT_EQ(3u, order.size());
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(2, order[1]);
  EXPECT_EQ(3, order[2]);

  order.clear();
  sig.emit(0);

  // Only 1 and 3 should fire
  ASSERT_EQ(2u, order.size());
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(3, order[1]);
}

// ============================================================================
// Disconnect Other Slot During Emission
// ============================================================================

TEST(EdgeCasesTest, DisconnectOtherSlot_DuringEmission) {
  signal<int> sig;
  std::vector<int> order;
  connection conn3;

  sig.connect([&](int) {
    order.push_back(1);
    conn3.disconnect(); // Disconnect slot 3
  });

  sig.connect([&](int) { order.push_back(2); });

  conn3 = sig.connect([&](int) { order.push_back(3); });

  sig.emit(0);

  // Slot 3 was disconnected before it was reached in priority order
  // Behavior: The slot was deactivated, snapshot was already taken
  // So slot 3 may or may not be called depending on when check happens

  // In our implementation, we check is_active() before each invocation
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(2, order[1]);
  // order[2] may or may not exist depending on timing
}

// ============================================================================
// Connect During Emission
// ============================================================================

TEST(EdgeCasesTest, ConnectDuringEmission_MayOrMayNotFire) {
  signal<int> sig;
  std::vector<int> order;

  sig.connect([&](int) {
    order.push_back(1);

    // Connect a new slot during emission
    sig.connect([&](int) { order.push_back(99); });
  });

  sig.connect([&](int) { order.push_back(2); });

  sig.emit(0);

  // The new slot may or may not fire in current emission
  // (implementation-defined per spec)
  EXPECT_GE(order.size(), 2u);
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(2, order[1]);

  // But it definitely fires on next emission
  order.clear();
  sig.emit(0);

  EXPECT_GE(order.size(), 3u);
}

// ============================================================================
// Empty Signal Edge Cases
// ============================================================================

TEST(EdgeCasesTest, EmptySignal_Operations) {
  signal<int> sig;

  // These should all be safe on empty signal
  sig.emit(42);
  sig.disconnect_all();
  sig.cleanup();

  EXPECT_TRUE(sig.empty());
  EXPECT_EQ(0u, sig.slot_count());
}

TEST(EdgeCasesTest, DisconnectAll_DuringEmission) {
  signal<int> sig;
  int call_count = 0;

  sig.connect([&](int) {
    ++call_count;
    sig.disconnect_all(); // Disconnect all during emission
  });

  sig.connect([&](int) { ++call_count; });
  sig.connect([&](int) { ++call_count; });

  sig.emit(0);

  // First slot fires and disconnects all
  // Other slots check is_active() and may not fire
  EXPECT_GE(call_count, 1);

  sig.emit(0);
  // No more slots
  EXPECT_GE(call_count, 1);
  EXPECT_EQ(0u, sig.slot_count());
}

// ============================================================================
// Block During Emission
// ============================================================================

TEST(EdgeCasesTest, BlockSelf_DuringEmission) {
  signal<int> sig;
  int call_count = 0;
  connection conn;

  conn = sig.connect([&](int) {
    ++call_count;
    conn.block(); // Block self for future emissions
  });

  sig.emit(0);
  EXPECT_EQ(1, call_count);

  sig.emit(0);
  EXPECT_EQ(1, call_count); // Blocked

  conn.unblock();
  sig.emit(0);
  EXPECT_EQ(2, call_count); // Unblocked
}

TEST(EdgeCasesTest, BlockOther_DuringEmission) {
  signal<int> sig;
  std::vector<int> order;
  connection conn2;

  sig.connect([&](int) {
    order.push_back(1);
    conn2.block(); // Block slot 2
  });

  conn2 = sig.connect([&](int) { order.push_back(2); });

  sig.connect([&](int) { order.push_back(3); });

  sig.emit(0);

  // Slot 2 may or may not fire (depends on timing of block check)
  EXPECT_EQ(1, order[0]);
  // Last element should be 3
  EXPECT_EQ(3, order.back());
}

// ============================================================================
// Signal Destruction Edge Cases
// ============================================================================

TEST(EdgeCasesTest, ConnectionAfterSignalDestroyed) {
  connection conn;

  {
    signal<int> sig;
    conn = sig.connect([](int) {});
    EXPECT_TRUE(conn.connected());
  } // sig destroyed

  // Connection should know it's invalid
  EXPECT_FALSE(conn.connected());

  // These should be safe
  conn.disconnect();
  conn.block();
  conn.unblock();
}

TEST(EdgeCasesTest, ScopedConnectionAfterSignalDestroyed) {
  scoped_connection sc;

  {
    signal<int> sig;
    sc = sig.connect([](int) {});
    EXPECT_TRUE(sc.connected());
  } // sig destroyed

  EXPECT_FALSE(sc.connected());
} // sc destroyed - should not crash

// ============================================================================
// Slot Throws (Exception Safety Note: exceptions disabled in HFT)
// ============================================================================

// Note: Since we compile with -fno-exceptions, we can't test exception
// propagation. In a non-HFT build, you would want to verify:
// - Slots can throw and other slots still get called
// - Signal state remains consistent after exception

// ============================================================================
// Stress Tests
// ============================================================================

TEST(EdgeCasesTest, ManySlotsConnect) {
  signal<int> sig;
  std::vector<connection> connections;
  int total = 0;

  for (int i = 0; i < 1000; ++i) {
    connections.push_back(sig.connect([&total](int v) { total += v; }));
  }

  EXPECT_EQ(1000u, sig.slot_count());

  sig.emit(1);

  EXPECT_EQ(1000, total);
}

TEST(EdgeCasesTest, RapidConnectDisconnect) {
  signal<int> sig;

  for (int i = 0; i < 1000; ++i) {
    auto conn = sig.connect([](int) {});
    sig.emit(i);
    conn.disconnect();
  }

  // Deactivated slots remain in list until cleanup
  sig.cleanup();
  EXPECT_EQ(0u, sig.slot_count());
}

TEST(EdgeCasesTest, AlternatingConnectEmitDisconnect) {
  signal<int> sig;
  int call_count = 0;

  for (int i = 0; i < 100; ++i) {
    auto conn = sig.connect([&call_count](int) { ++call_count; });
    sig.emit(0);
    conn.disconnect();
    sig.emit(0); // Should not increment
  }

  EXPECT_EQ(100, call_count);
}

} // namespace test
} // namespace fb
