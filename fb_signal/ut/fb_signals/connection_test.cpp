/// @file connection_test.cpp
/// @brief Unit tests for connection lifecycle and RAII behavior

#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace fb {
namespace test {

// ============================================================================
// Basic Connection Tests
// ============================================================================

TEST(ConnectionTest, DefaultConnection_IsInvalid) {
  connection conn;

  EXPECT_FALSE(conn.valid());
  EXPECT_FALSE(conn.connected());
  EXPECT_EQ(0u, conn.id());
}

TEST(ConnectionTest, ValidConnection_ReportsConnected) {
  signal<int> sig;

  auto conn = sig.connect([](int) {});

  EXPECT_TRUE(conn.valid());
  EXPECT_TRUE(conn.connected());
  EXPECT_NE(0u, conn.id());
}

TEST(ConnectionTest, Disconnect_StopsInvocation) {
  signal<int> sig;
  int call_count = 0;

  auto conn = sig.connect([&call_count](int) { ++call_count; });

  sig.emit(0);
  EXPECT_EQ(1, call_count);

  conn.disconnect();

  EXPECT_FALSE(conn.connected());

  sig.emit(0);
  EXPECT_EQ(1, call_count); // Not called again
}

TEST(ConnectionTest, Disconnect_MultipleTimes_Safe) {
  signal<int> sig;

  auto conn = sig.connect([](int) {});

  conn.disconnect();
  conn.disconnect(); // Should not crash
  conn.disconnect();

  EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, Disconnect_InvalidConnection_Safe) {
  connection conn;

  conn.disconnect(); // Should not crash

  EXPECT_FALSE(conn.connected());
}

// ============================================================================
// Connection Blocking Tests
// ============================================================================

TEST(ConnectionTest, Block_PreventsInvocation) {
  signal<int> sig;
  int call_count = 0;

  auto conn = sig.connect([&call_count](int) { ++call_count; });

  sig.emit(0);
  EXPECT_EQ(1, call_count);

  conn.block();
  EXPECT_TRUE(conn.blocked());

  sig.emit(0);
  EXPECT_EQ(1, call_count); // Not called while blocked
}

TEST(ConnectionTest, Unblock_ResumesInvocation) {
  signal<int> sig;
  int call_count = 0;

  auto conn = sig.connect([&call_count](int) { ++call_count; });

  conn.block();
  sig.emit(0);
  EXPECT_EQ(0, call_count);

  conn.unblock();
  EXPECT_FALSE(conn.blocked());

  sig.emit(0);
  EXPECT_EQ(1, call_count);
}

TEST(ConnectionTest, Block_InvalidConnection_Safe) {
  connection conn;

  conn.block();   // Should not crash
  conn.unblock(); // Should not crash

  EXPECT_FALSE(conn.blocked());
}

// ============================================================================
// Connection Equality Tests
// ============================================================================

TEST(ConnectionTest, Equality_SameConnection_Equal) {
  signal<int> sig;

  auto conn1 = sig.connect([](int) {});
  auto conn2 = conn1; // Copy

  EXPECT_EQ(conn1, conn2);
}

TEST(ConnectionTest, Equality_DifferentConnections_NotEqual) {
  signal<int> sig;

  auto conn1 = sig.connect([](int) {});
  auto conn2 = sig.connect([](int) {});

  EXPECT_NE(conn1, conn2);
}

TEST(ConnectionTest, Equality_BothInvalid_Equal) {
  connection conn1;
  connection conn2;

  EXPECT_EQ(conn1, conn2);
}

// ============================================================================
// Scoped Connection Tests
// ============================================================================

TEST(ScopedConnectionTest, DefaultConstruct_IsEmpty) {
  scoped_connection sc;

  EXPECT_FALSE(sc.connected());
}

TEST(ScopedConnectionTest, Destruction_DisconnectsSlot) {
  signal<int> sig;
  int call_count = 0;

  {
    scoped_connection sc = sig.connect([&call_count](int) { ++call_count; });

    sig.emit(0);
    EXPECT_EQ(1, call_count);
  } // sc destroyed here

  sig.emit(0);
  EXPECT_EQ(1, call_count); // Slot was disconnected
}

TEST(ScopedConnectionTest, MoveConstruct_TransfersOwnership) {
  signal<int> sig;
  int call_count = 0;

  scoped_connection sc1 = sig.connect([&call_count](int) { ++call_count; });
  scoped_connection sc2(std::move(sc1));

  sig.emit(0);
  EXPECT_EQ(1, call_count);

  EXPECT_TRUE(sc2.connected());
}

TEST(ScopedConnectionTest, MoveAssign_DisconnectsOldAndTransfers) {
  signal<int> sig;
  int count1 = 0;
  int count2 = 0;

  scoped_connection sc1 = sig.connect([&count1](int) { ++count1; });
  scoped_connection sc2 = sig.connect([&count2](int) { ++count2; });

  sc1 = std::move(sc2);

  sig.emit(0);

  EXPECT_EQ(0, count1); // sc1's original slot disconnected
  EXPECT_EQ(1, count2); // sc2's slot still works via sc1
}

TEST(ScopedConnectionTest, Release_ReturnsConnection) {
  signal<int> sig;
  int call_count = 0;

  scoped_connection sc = sig.connect([&call_count](int) { ++call_count; });

  connection conn = sc.release();

  EXPECT_FALSE(sc.connected());
  EXPECT_TRUE(conn.connected());

  // sc is now empty, but conn still works
  sig.emit(0);
  EXPECT_EQ(1, call_count);
}

TEST(ScopedConnectionTest, BlockUnblock_Works) {
  signal<int> sig;
  int call_count = 0;

  scoped_connection sc = sig.connect([&call_count](int) { ++call_count; });

  sc.block();
  sig.emit(0);
  EXPECT_EQ(0, call_count);

  sc.unblock();
  sig.emit(0);
  EXPECT_EQ(1, call_count);
}

// ============================================================================
// Connection Guard Tests
// ============================================================================

TEST(ConnectionGuardTest, AddConnections_AllTracked) {
  signal<int> sig;
  connection_guard guard;

  guard += sig.connect([](int) {});
  guard += sig.connect([](int) {});
  guard.add(sig.connect([](int) {}));

  EXPECT_EQ(3u, guard.size());
  EXPECT_FALSE(guard.empty());
}

TEST(ConnectionGuardTest, Destruction_DisconnectsAll) {
  signal<int> sig;
  int call_count = 0;

  {
    connection_guard guard;
    guard += sig.connect([&call_count](int) { ++call_count; });
    guard += sig.connect([&call_count](int) { ++call_count; });
    guard += sig.connect([&call_count](int) { ++call_count; });

    sig.emit(0);
    EXPECT_EQ(3, call_count);
  } // guard destroyed

  sig.emit(0);
  EXPECT_EQ(3, call_count); // No additional calls
}

TEST(ConnectionGuardTest, DisconnectAll_Manual) {
  signal<int> sig;
  connection_guard guard;

  guard += sig.connect([](int) {});
  guard += sig.connect([](int) {});

  EXPECT_EQ(2u, guard.size());

  guard.disconnect_all();

  EXPECT_TRUE(guard.empty());
}

TEST(ConnectionGuardTest, BlockUnblockAll_Works) {
  signal<int> sig;
  int call_count = 0;

  connection_guard guard;
  guard += sig.connect([&call_count](int) { ++call_count; });
  guard += sig.connect([&call_count](int) { ++call_count; });

  guard.block_all();
  sig.emit(0);
  EXPECT_EQ(0, call_count);

  guard.unblock_all();
  sig.emit(0);
  EXPECT_EQ(2, call_count);
}

TEST(ConnectionGuardTest, Cleanup_RemovesDisconnected) {
  signal<int> sig;
  connection_guard guard;

  auto conn1 = sig.connect([](int) {});
  auto conn2 = sig.connect([](int) {});
  auto conn3 = sig.connect([](int) {});

  guard.add(conn1);
  guard.add(conn2);
  guard.add(conn3);

  EXPECT_EQ(3u, guard.size());

  conn2.disconnect();
  guard.cleanup();

  EXPECT_EQ(2u, guard.size());
}

// ============================================================================
// Connection Blocker Tests
// ============================================================================

TEST(ConnectionBlockerTest, BlocksOnConstruct_UnblocksOnDestruct) {
  signal<int> sig;
  int call_count = 0;

  auto conn = sig.connect([&call_count](int) { ++call_count; });

  {
    connection_blocker blocker(conn);

    sig.emit(0);
    EXPECT_EQ(0, call_count);
  } // blocker destroyed

  sig.emit(0);
  EXPECT_EQ(1, call_count);
}

TEST(ConnectionBlockerTest, AlreadyBlocked_StaysBlocked) {
  signal<int> sig;
  int call_count = 0;

  auto conn = sig.connect([&call_count](int) { ++call_count; });
  conn.block();

  {
    connection_blocker blocker(conn);
    // Already blocked
  } // blocker destroyed

  // Should still be blocked (was blocked before blocker)
  EXPECT_TRUE(conn.blocked());

  sig.emit(0);
  EXPECT_EQ(0, call_count);
}

// ============================================================================
// Connection Copy Semantics Tests
// ============================================================================

TEST(ConnectionTest, Copy_SharesSameSlot) {
  signal<int> sig;
  int call_count = 0;

  auto conn1 = sig.connect([&call_count](int) { ++call_count; });
  auto conn2 = conn1;

  EXPECT_EQ(conn1.id(), conn2.id());

  conn1.disconnect();

  EXPECT_FALSE(conn1.connected());
  EXPECT_FALSE(conn2.connected()); // Same slot
}

} // namespace test
} // namespace fb
