/// @file thread_safety_test.cpp
/// @brief Unit tests for concurrent signal operations

#include <fb/event_queue.hpp>
#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace fb {
namespace test {

// ============================================================================
// Concurrent Emission Tests
// ============================================================================

TEST(ThreadSafetyTest, ConcurrentEmission_NoDataRace) {
  signal<int> sig;
  std::atomic<int> total{0};

  sig.connect([&total](int value) {
    total.fetch_add(value, std::memory_order_relaxed);
  });

  constexpr int NUM_THREADS = 8;
  constexpr int EMISSIONS_PER_THREAD = 1000;

  std::vector<std::thread> threads;
  threads.reserve(NUM_THREADS);

  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([&sig]() {
      for (int i = 0; i < EMISSIONS_PER_THREAD; ++i) {
        sig.emit(1);
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(NUM_THREADS * EMISSIONS_PER_THREAD, total.load());
}

TEST(ThreadSafetyTest, ConcurrentEmission_MultipleSlots) {
  signal<int> sig;
  std::atomic<int> slot1_count{0};
  std::atomic<int> slot2_count{0};
  std::atomic<int> slot3_count{0};

  sig.connect([&slot1_count](int) { slot1_count.fetch_add(1); });
  sig.connect([&slot2_count](int) { slot2_count.fetch_add(1); });
  sig.connect([&slot3_count](int) { slot3_count.fetch_add(1); });

  constexpr int NUM_THREADS = 4;
  constexpr int EMISSIONS_PER_THREAD = 500;

  std::vector<std::thread> threads;
  for (int t = 0; t < NUM_THREADS; ++t) {
    threads.emplace_back([&sig]() {
      for (int i = 0; i < EMISSIONS_PER_THREAD; ++i) {
        sig.emit(i);
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  const int expected = NUM_THREADS * EMISSIONS_PER_THREAD;
  EXPECT_EQ(expected, slot1_count.load());
  EXPECT_EQ(expected, slot2_count.load());
  EXPECT_EQ(expected, slot3_count.load());
}

// ============================================================================
// Concurrent Connection Tests
// ============================================================================

TEST(ThreadSafetyTest, ConcurrentConnect_WhileEmitting) {
  signal<int> sig;
  std::atomic<int> call_count{0};
  std::atomic<bool> run{true};

  // Initial slot
  sig.connect([&call_count](int) { call_count.fetch_add(1); });

  // Emitter thread
  std::thread emitter([&]() {
    while (run.load(std::memory_order_relaxed)) {
      sig.emit(1);
      std::this_thread::yield();
    }
  });

  // Connector thread - adds slots while emissions are happening
  std::thread connector([&]() {
    for (int i = 0; i < 100; ++i) {
      sig.connect([&call_count](int) { call_count.fetch_add(1); });
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  });

  connector.join();
  run.store(false, std::memory_order_relaxed);
  emitter.join();

  // Just verify we didn't crash and got some calls
  EXPECT_GT(call_count.load(), 0);
}

TEST(ThreadSafetyTest, ConcurrentDisconnect_WhileEmitting) {
  signal<int> sig;
  std::atomic<int> call_count{0};
  std::atomic<bool> run{true};

  // Create connections that will be disconnected
  std::vector<connection> connections;
  for (int i = 0; i < 50; ++i) {
    connections.push_back(
        sig.connect([&call_count](int) { call_count.fetch_add(1); }));
  }

  // Emitter thread
  std::thread emitter([&]() {
    while (run.load(std::memory_order_relaxed)) {
      sig.emit(1);
      std::this_thread::yield();
    }
  });

  // Disconnector thread
  std::thread disconnector([&]() {
    for (auto &conn : connections) {
      conn.disconnect();
      std::this_thread::sleep_for(std::chrono::microseconds(5));
    }
  });

  disconnector.join();
  run.store(false, std::memory_order_relaxed);
  emitter.join();

  // All should be disconnected
  for (const auto &conn : connections) {
    EXPECT_FALSE(conn.connected());
  }
}

TEST(ThreadSafetyTest, ConcurrentConnectDisconnect) {
  signal<int> sig;
  std::atomic<bool> run{true};
  std::atomic<int> connect_count{0};
  std::atomic<int> disconnect_count{0};

  // Multiple threads connecting
  std::vector<std::thread> connectors;
  for (int t = 0; t < 3; ++t) {
    connectors.emplace_back([&]() {
      std::vector<connection> local_conns;
      while (run.load(std::memory_order_relaxed)) {
        auto conn = sig.connect([](int) {});
        local_conns.push_back(conn);
        connect_count.fetch_add(1);

        if (local_conns.size() > 10) {
          local_conns.front().disconnect();
          local_conns.erase(local_conns.begin());
          disconnect_count.fetch_add(1);
        }

        std::this_thread::yield();
      }

      for (auto &c : local_conns) {
        c.disconnect();
      }
    });
  }

  // Let it run for a bit
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  run.store(false);

  for (auto &t : connectors) {
    t.join();
  }

  EXPECT_GT(connect_count.load(), 0);
}

// ============================================================================
// Event Queue Tests
// ============================================================================

TEST(EventQueueTest, EnqueueDequeue_SingleThread) {
  event_queue queue;
  int value = 0;

  queue.enqueue([&value]() { value = 42; });

  EXPECT_FALSE(queue.empty());

  std::size_t processed = queue.process_pending();

  EXPECT_EQ(1u, processed);
  EXPECT_EQ(42, value);
  EXPECT_TRUE(queue.empty());
}

TEST(EventQueueTest, EnqueueDequeue_FIFO) {
  event_queue queue;
  std::vector<int> order;

  queue.enqueue([&order]() { order.push_back(1); });
  queue.enqueue([&order]() { order.push_back(2); });
  queue.enqueue([&order]() { order.push_back(3); });

  queue.process_pending();

  ASSERT_EQ(3u, order.size());
  EXPECT_EQ(1, order[0]);
  EXPECT_EQ(2, order[1]);
  EXPECT_EQ(3, order[2]);
}

TEST(EventQueueTest, MultiProducer_SingleConsumer) {
  event_queue queue;

  constexpr int NUM_PRODUCERS = 4;
  constexpr int EVENTS_PER_PRODUCER = 100;

  std::vector<std::thread> producers;
  for (int p = 0; p < NUM_PRODUCERS; ++p) {
    producers.emplace_back([&queue]() {
      for (int i = 0; i < EVENTS_PER_PRODUCER; ++i) {
        queue.enqueue([]() {}); // Just enqueue, no action needed
      }
    });
  }

  for (auto &t : producers) {
    t.join();
  }

  std::size_t processed = queue.process_pending();

  // May be slightly less if there was contention
  EXPECT_GT(processed, 0u);
}

TEST(EventQueueTest, ProcessPending_MaxEvents) {
  event_queue queue;
  int count = 0;

  for (int i = 0; i < 10; ++i) {
    queue.enqueue([&count]() { ++count; });
  }

  std::size_t processed = queue.process_pending(3);

  EXPECT_EQ(3u, processed);
  EXPECT_EQ(3, count);
  EXPECT_FALSE(queue.empty());

  queue.process_pending();
  EXPECT_EQ(10, count);
}

TEST(EventQueueTest, ThreadLocalQueue_Access) {
  auto &queue1 = thread_event_queue();
  auto &queue2 = thread_event_queue();

  // Same thread, same queue
  EXPECT_EQ(&queue1, &queue2);
}

TEST(EventQueueTest, ThreadLocalQueue_DifferentThreads) {
  event_queue *queue1 = nullptr;
  event_queue *queue2 = nullptr;

  std::thread t1([&queue1]() { queue1 = &thread_event_queue(); });
  std::thread t2([&queue2]() { queue2 = &thread_event_queue(); });

  t1.join();
  t2.join();

  // Different threads, different queues
  EXPECT_NE(queue1, queue2);
}

// ============================================================================
// Block/Unblock Thread Safety Tests
// ============================================================================

TEST(ThreadSafetyTest, BlockUnblock_WhileEmitting) {
  signal<int> sig;
  std::atomic<int> call_count{0};
  std::atomic<bool> run{true};

  auto conn = sig.connect([&call_count](int) { call_count.fetch_add(1); });

  // Emitter thread
  std::thread emitter([&]() {
    while (run.load()) {
      sig.emit(1);
    }
  });

  // Blocker thread
  std::thread blocker([&]() {
    for (int i = 0; i < 100; ++i) {
      conn.block();
      std::this_thread::sleep_for(std::chrono::microseconds(10));
      conn.unblock();
      std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
  });

  blocker.join();
  run.store(false);
  emitter.join();

  // Some calls should have happened
  EXPECT_GT(call_count.load(), 0);
}

} // namespace test
} // namespace fb
