/// @file queued_delivery_test.cpp
/// @brief Unit tests for queued and automatic delivery policies

#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace fb {
namespace test {

// ============================================================================
// Queued Delivery Tests
// ============================================================================

TEST(QueuedDeliveryTest, QueuedConnection_DefersInvocation)
{
  signal<int> sig;
  event_queue queue;
  std::atomic<int> received{0};

  sig.connect(
      [&received](int value)
      {
        received.store(value, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      queue);

  // Emit should not invoke immediately
  sig.emit(42);

  // Value should still be 0 (not yet processed)
  EXPECT_EQ(0, received.load());

  // Process the queued event
  std::size_t processed = queue.process_pending();

  EXPECT_EQ(1u, processed);
  EXPECT_EQ(42, received.load());
}

TEST(QueuedDeliveryTest, QueuedConnection_MultipleEmissions)
{
  signal<int> sig;
  event_queue queue;
  std::vector<int> received;

  sig.connect(
      [&received](int value)
      {
        received.push_back(value);
      },
      delivery_policy::queued,
      queue);

  sig.emit(1);
  sig.emit(2);
  sig.emit(3);

  EXPECT_TRUE(received.empty());

  queue.process_pending();

  ASSERT_EQ(3u, received.size());
  EXPECT_EQ(1, received[0]);
  EXPECT_EQ(2, received[1]);
  EXPECT_EQ(3, received[2]);
}

TEST(QueuedDeliveryTest, QueuedConnection_FIFO_Order)
{
  signal<int> sig;
  event_queue queue;
  std::vector<int> order;

  sig.connect(
      [&order](int v)
      {
        order.push_back(v);
      },
      delivery_policy::queued,
      queue);

  for (int i = 0; i < 10; ++i)
  {
    sig.emit(i);
  }

  queue.process_pending();

  ASSERT_EQ(10u, order.size());
  for (int i = 0; i < 10; ++i)
  {
    EXPECT_EQ(i, order[static_cast<std::size_t>(i)]);
  }
}

TEST(QueuedDeliveryTest, MixedDeliveryPolicies)
{
  signal<int> sig;
  event_queue queue;
  std::vector<std::string> order;

  // Direct slot
  sig.connect([&order](int)
  {
    order.push_back("direct");
  });

  // Queued slot
  sig.connect(
      [&order](int)
      {
        order.push_back("queued");
      },
      delivery_policy::queued,
      queue);

  sig.emit(1);

  // Direct should have fired, queued should not
  ASSERT_EQ(1u, order.size());
  EXPECT_EQ("direct", order[0]);

  // Process queued
  queue.process_pending();

  ASSERT_EQ(2u, order.size());
  EXPECT_EQ("queued", order[1]);
}

// ============================================================================
// Automatic Delivery Tests
// ============================================================================

TEST(AutomaticDeliveryTest, SameThread_DirectDelivery)
{
  signal<int> sig;
  event_queue queue;  // Created on current thread
  std::atomic<int> received{0};

  sig.connect(
      [&received](int value)
      {
        received.store(value, std::memory_order_relaxed);
      },
      delivery_policy::automatic,
      queue);

  // Emit from the same thread that owns the queue
  sig.emit(42);

  // Should be invoked directly (same thread)
  EXPECT_EQ(42, received.load());

  // Queue should be empty
  EXPECT_TRUE(queue.empty());
}

TEST(AutomaticDeliveryTest, DifferentThread_QueuedDelivery)
{
  signal<int> sig;
  event_queue queue;  // Created on main thread
  std::atomic<int> received{0};

  sig.connect(
      [&received](int value)
      {
        received.store(value, std::memory_order_relaxed);
      },
      delivery_policy::automatic,
      queue);

  // Emit from a different thread
  std::thread emitter([&sig]()
  {
    sig.emit(42);
  });
  emitter.join();

  // Should NOT be invoked yet (different thread)
  EXPECT_EQ(0, received.load());

  // Process the queued event
  queue.process_pending();

  EXPECT_EQ(42, received.load());
}

// ============================================================================
// Cross-Thread Communication Tests
// ============================================================================

TEST(CrossThreadTest, ProducerConsumer_Pattern)
{
  signal<int> sig;
  event_queue consumer_queue;
  std::atomic<int> sum{0};
  std::atomic<bool> producer_done{false};

  // Connect with queued delivery
  sig.connect(
      [&sum](int value)
      {
        sum.fetch_add(value, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      consumer_queue);

  // Producer thread
  std::thread producer([&]()
  {
    for (int i = 1; i <= 100; ++i)
    {
      sig.emit(i);
    }
    producer_done.store(true, std::memory_order_release);
  });

  // Consumer loop (main thread)
  while (!producer_done.load(std::memory_order_acquire) ||
         !consumer_queue.empty())
  {
    consumer_queue.process_pending();
    std::this_thread::yield();
  }

  producer.join();

  // Sum of 1..100 = 5050
  EXPECT_EQ(5050, sum.load());
}

TEST(CrossThreadTest, MultipleProducers_SingleConsumer)
{
  signal<int> sig;
  event_queue consumer_queue;
  std::atomic<int> total{0};
  std::atomic<int> producers_done{0};

  constexpr int NUM_PRODUCERS = 4;
  constexpr int EVENTS_PER_PRODUCER = 50;

  sig.connect(
      [&total](int value)
      {
        total.fetch_add(value, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      consumer_queue);

  // Start producer threads
  std::vector<std::thread> producers;
  for (int p = 0; p < NUM_PRODUCERS; ++p)
  {
    producers.emplace_back([&sig, &producers_done]()
    {
      for (int i = 0; i < EVENTS_PER_PRODUCER; ++i)
      {
        sig.emit(1);
      }
      producers_done.fetch_add(1, std::memory_order_release);
    });
  }

  // Consumer loop
  while (producers_done.load(std::memory_order_acquire) < NUM_PRODUCERS ||
         !consumer_queue.empty())
  {
    consumer_queue.process_pending();
    std::this_thread::yield();
  }

  for (auto &t : producers)
  {
    t.join();
  }

  EXPECT_EQ(NUM_PRODUCERS * EVENTS_PER_PRODUCER, total.load());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(QueuedDeliveryTest, DisconnectBeforeProcessing)
{
  signal<int> sig;
  event_queue queue;
  std::atomic<int> count{0};

  auto conn = sig.connect(
      [&count](int)
      {
        count.fetch_add(1, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      queue);

  sig.emit(1);
  sig.emit(2);

  // Disconnect before processing
  conn.disconnect();

  // Process should not invoke (slot is deactivated)
  queue.process_pending();

  EXPECT_EQ(0, count.load());
}

TEST(QueuedDeliveryTest, BlockBeforeProcessing)
{
  signal<int> sig;
  event_queue queue;
  std::atomic<int> count{0};

  auto conn = sig.connect(
      [&count](int)
      {
        count.fetch_add(1, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      queue);

  sig.emit(1);

  // Block before processing
  conn.block();

  // Process should not invoke (slot is blocked)
  queue.process_pending();

  EXPECT_EQ(0, count.load());

  // Unblock and emit again
  conn.unblock();
  sig.emit(2);
  queue.process_pending();

  EXPECT_EQ(1, count.load());
}

TEST(QueuedDeliveryTest, VoidSignal_QueuedDelivery)
{
  signal<> sig;
  event_queue queue;
  std::atomic<int> count{0};

  sig.connect(
      [&count]()
      {
        count.fetch_add(1, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      queue);

  sig.emit();
  sig.emit();
  sig.emit();

  EXPECT_EQ(0, count.load());

  queue.process_pending();

  EXPECT_EQ(3, count.load());
}

TEST(QueuedDeliveryTest, ComplexArguments_QueuedDelivery)
{
  signal<int, std::string, double> sig;
  event_queue queue;

  int received_int = 0;
  std::string received_str;
  double received_double = 0.0;

  sig.connect(
      [&](int i, std::string s, double d)
      {
        received_int = i;
        received_str = std::move(s);
        received_double = d;
      },
      delivery_policy::queued,
      queue);

  sig.emit(42, "hello", 3.14);

  EXPECT_EQ(0, received_int);

  queue.process_pending();

  EXPECT_EQ(42, received_int);
  EXPECT_EQ("hello", received_str);
  EXPECT_DOUBLE_EQ(3.14, received_double);
}

TEST(QueuedDeliveryTest, ProcessPending_MaxEvents)
{
  signal<int> sig;
  event_queue queue;
  std::atomic<int> count{0};

  sig.connect(
      [&count](int)
      {
        count.fetch_add(1, std::memory_order_relaxed);
      },
      delivery_policy::queued,
      queue);

  for (int i = 0; i < 10; ++i)
  {
    sig.emit(i);
  }

  // Process only 3 events
  std::size_t processed = queue.process_pending(3);

  EXPECT_EQ(3u, processed);
  EXPECT_EQ(3, count.load());
  EXPECT_FALSE(queue.empty());

  // Process remaining
  queue.process_pending();
  EXPECT_EQ(10, count.load());
}

// ============================================================================
// Priority with Queued Delivery
// ============================================================================

TEST(QueuedDeliveryTest, PriorityOrdering_Preserved)
{
  signal<int> sig;
  event_queue queue;
  std::vector<int> order;

  sig.connect(
      [&order](int)
      {
        order.push_back(1);
      },
      delivery_policy::queued,
      queue,
      priority::low);

  sig.connect(
      [&order](int)
      {
        order.push_back(2);
      },
      delivery_policy::queued,
      queue,
      priority::high);

  sig.connect(
      [&order](int)
      {
        order.push_back(3);
      },
      delivery_policy::queued,
      queue,
      priority::normal);

  sig.emit(0);

  queue.process_pending();

  // Events should be queued in priority order
  ASSERT_EQ(3u, order.size());
  EXPECT_EQ(2, order[0]);  // high
  EXPECT_EQ(3, order[1]);  // normal
  EXPECT_EQ(1, order[2]);  // low
}

} // namespace test
} // namespace fb
