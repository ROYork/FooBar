/// @file test_timer.cpp
/// @brief Comprehensive unit tests for timer class
///
/// Tests all timer functionality including:
/// - Basic start/stop operations
/// - Single-shot and repeating modes
/// - Interval configuration
/// - Thread safety
/// - Signal emission
/// - Edge cases and error handling

#include <gtest/gtest.h>
#include <fb/timer.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

using namespace fb;
using namespace std::chrono_literals;

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(TimerTest, DefaultConstruction)
{
    timer t;

    EXPECT_EQ(t.interval(), 0);
    EXPECT_FALSE(t.is_active());
    EXPECT_FALSE(t.is_single_shot());
    EXPECT_EQ(t.remaining_time(), 0);
}

TEST(TimerTest, SetAndGetInterval)
{
    timer t;

    t.set_interval(1000);
    EXPECT_EQ(t.interval(), 1000);

    t.set_interval(500);
    EXPECT_EQ(t.interval(), 500);
}

TEST(TimerTest, InvalidInterval)
{
    timer t;

    EXPECT_THROW(t.set_interval(0), std::invalid_argument);
    EXPECT_THROW(t.set_interval(-100), std::invalid_argument);
    EXPECT_THROW(t.start(0), std::invalid_argument);
    EXPECT_THROW(t.start(-50), std::invalid_argument);
}

TEST(TimerTest, SingleShotMode)
{
    timer t;

    EXPECT_FALSE(t.is_single_shot());

    t.set_single_shot(true);
    EXPECT_TRUE(t.is_single_shot());

    t.set_single_shot(false);
    EXPECT_FALSE(t.is_single_shot());
}

// ============================================================================
// Start/Stop Tests
// ============================================================================

TEST(TimerTest, StartWithInterval)
{
    timer t;

    t.start(100);

    EXPECT_TRUE(t.is_active());
    EXPECT_EQ(t.interval(), 100);

    t.stop();
    EXPECT_FALSE(t.is_active());
}

TEST(TimerTest, StartWithoutInterval)
{
    timer t;

    EXPECT_THROW(t.start(), std::runtime_error);
}

TEST(TimerTest, StartAfterSetInterval)
{
    timer t;

    t.set_interval(100);
    t.start();

    EXPECT_TRUE(t.is_active());
    EXPECT_EQ(t.interval(), 100);

    t.stop();
}

TEST(TimerTest, StopInactiveTimer)
{
    timer t;

    // Stopping inactive timer should be no-op
    EXPECT_NO_THROW(t.stop());
    EXPECT_FALSE(t.is_active());
}

TEST(TimerTest, Restart)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    t.start(50);
    std::this_thread::sleep_for(75ms);

    int countBefore = count.load();
    EXPECT_GT(countBefore, 0);

    // Restart should reset the timer
    t.restart();
    count = 0;

    std::this_thread::sleep_for(75ms);
    EXPECT_GT(count.load(), 0);

    t.stop();
}

TEST(TimerTest, MultipleStartsRestartsTimer)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    t.start(100);
    std::this_thread::sleep_for(50ms);

    // Start again should restart timer
    t.start(100);
    std::this_thread::sleep_for(50ms);

    // Should have fired 0 times (restarted before first timeout)
    EXPECT_EQ(count.load(), 0);

    std::this_thread::sleep_for(60ms);
    // Now should have fired at least once
    EXPECT_GT(count.load(), 0);

    t.stop();
}

// ============================================================================
// Signal Emission Tests
// ============================================================================

TEST(TimerTest, TimeoutSignalEmitted)
{
    timer t;
    std::atomic<bool> fired{false};

    t.timeout.connect([&fired]() {
        fired = true;
    });

    t.start(50);
    std::this_thread::sleep_for(80ms);

    EXPECT_TRUE(fired.load());
    t.stop();
}

TEST(TimerTest, RepeatingTimer)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    t.start(50);
    std::this_thread::sleep_for(180ms);

    int finalCount = count.load();
    EXPECT_GE(finalCount, 3);  // Should fire at least 3 times in 180ms
    EXPECT_LE(finalCount, 5);  // But not too many times

    t.stop();
}

TEST(TimerTest, SingleShotTimerFiresOnce)
{
    timer t;
    std::atomic<int> count{0};

    t.set_single_shot(true);
    t.timeout.connect([&count]() {
        count++;
    });

    t.start(50);
    std::this_thread::sleep_for(150ms);

    EXPECT_EQ(count.load(), 1);  // Should fire exactly once
    EXPECT_FALSE(t.is_active());  // Should auto-stop
}

TEST(TimerTest, MultipleSlots)
{
    timer t;
    std::atomic<int> count1{0};
    std::atomic<int> count2{0};
    std::atomic<int> count3{0};

    t.timeout.connect([&count1]() { count1++; });
    t.timeout.connect([&count2]() { count2++; });
    t.timeout.connect([&count3]() { count3++; });

    t.start(50);
    std::this_thread::sleep_for(80ms);

    EXPECT_GT(count1.load(), 0);
    EXPECT_GT(count2.load(), 0);
    EXPECT_GT(count3.load(), 0);

    t.stop();
}

// ============================================================================
// Timing Accuracy Tests
// ============================================================================

TEST(TimerTest, TimingAccuracy)
{
    timer t;
    std::atomic<bool> fired{false};
    auto startTime = std::chrono::steady_clock::now();

    t.set_single_shot(true);
    t.timeout.connect([&fired, &startTime]() {
        auto elapsed = std::chrono::steady_clock::now() - startTime;
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        // Should fire between 90ms and 110ms (allowing for system scheduling)
        EXPECT_GE(elapsedMs, 90);
        EXPECT_LE(elapsedMs, 110);

        fired = true;
    });

    t.start(100);
    std::this_thread::sleep_for(150ms);

    EXPECT_TRUE(fired.load());
}

TEST(TimerTest, RemainingTime)
{
    timer t;

    t.start(1000);

    // Check remaining time
    std::this_thread::sleep_for(100ms);
    int remaining = t.remaining_time();

    EXPECT_GT(remaining, 0);
    EXPECT_LT(remaining, 1000);
    EXPECT_GE(remaining, 850);  // Should have ~900ms left (allowing variance)

    t.stop();

    // After stop, remaining time should be 0
    EXPECT_EQ(t.remaining_time(), 0);
}

TEST(TimerTest, RemainingTimeInactiveTimer)
{
    timer t;

    EXPECT_EQ(t.remaining_time(), 0);

    t.set_interval(1000);
    EXPECT_EQ(t.remaining_time(), 0);  // Still not started
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(TimerTest, StartStopFromMultipleThreads)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    // Thread 1: Start and stop repeatedly
    std::thread t1([&t]() {
        for (int i = 0; i < 10; ++i) {
            t.start(10);
            std::this_thread::sleep_for(5ms);
            t.stop();
        }
    });

    // Thread 2: Start and stop repeatedly
    std::thread t2([&t]() {
        for (int i = 0; i < 10; ++i) {
            t.start(10);
            std::this_thread::sleep_for(5ms);
            t.stop();
        }
    });

    t1.join();
    t2.join();

    // Should not crash and timer should be stopped
    EXPECT_FALSE(t.is_active());
}

TEST(TimerTest, SetIntervalWhileRunning)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    t.start(100);
    std::this_thread::sleep_for(50ms);

    // Change interval while running (should restart)
    t.set_interval(50);

    std::this_thread::sleep_for(100ms);

    EXPECT_GT(count.load(), 0);
    t.stop();
}

TEST(TimerTest, StopFromTimeoutSlot)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&t, &count]() {
        count++;
        if (count >= 3) {
            t.stop();
        }
    });

    t.start(30);
    std::this_thread::sleep_for(150ms);

    EXPECT_EQ(count.load(), 3);
    EXPECT_FALSE(t.is_active());
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST(TimerTest, VeryShortInterval)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    // 1ms interval
    t.start(1);
    std::this_thread::sleep_for(50ms);

    // Should fire many times
    EXPECT_GT(count.load(), 10);

    t.stop();
}

TEST(TimerTest, LongInterval)
{
    timer t;
    std::atomic<bool> fired{false};

    t.set_single_shot(true);
    t.timeout.connect([&fired]() {
        fired = true;
    });

    // 2 second interval
    t.start(2000);

    // Check it hasn't fired yet
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(fired.load());
    EXPECT_TRUE(t.is_active());

    // Stop before it fires
    t.stop();
    EXPECT_FALSE(t.is_active());

    // Wait and verify it never fired
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(fired.load());
}

TEST(TimerTest, RestartFromTimeoutSlot)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&t, &count]() {
        count++;
        if (count == 1) {
            t.restart();
        }
    });

    t.start(50);
    std::this_thread::sleep_for(200ms);

    // Should have restarted and fired multiple times
    EXPECT_GT(count.load(), 2);

    t.stop();
}

TEST(TimerTest, DestructorStopsTimer)
{
    std::atomic<int> count{0};

    {
        timer t;
        t.timeout.connect([&count]() {
            count++;
        });

        t.start(30);
        std::this_thread::sleep_for(60ms);

        EXPECT_GT(count.load(), 0);
    }  // timer destroyed here

    int countBeforeWait = count.load();
    std::this_thread::sleep_for(100ms);

    // Count should not increase after timer destroyed
    EXPECT_EQ(count.load(), countBeforeWait);
}

// ============================================================================
// Non-Movable Tests
// ============================================================================

TEST(TimerTest, NonCopyable)
{
    // timer should not be copyable
    EXPECT_FALSE(std::is_copy_constructible<timer>::value);
    EXPECT_FALSE(std::is_copy_assignable<timer>::value);
}

TEST(TimerTest, NonMovable)
{
    // timer should not be movable (because signal<> is non-movable)
    EXPECT_FALSE(std::is_move_constructible<timer>::value);
    EXPECT_FALSE(std::is_move_assignable<timer>::value);
}

// ============================================================================
// Static Method Tests
// ============================================================================

TEST(TimerTest, SingleShotStaticMethod)
{
    std::atomic<bool> fired{false};

    auto t = timer::single_shot(50, [&fired]() {
        fired = true;
    });

    EXPECT_TRUE(t->is_active());
    EXPECT_TRUE(t->is_single_shot());

    std::this_thread::sleep_for(80ms);

    EXPECT_TRUE(fired.load());
    EXPECT_FALSE(t->is_active());
}

TEST(TimerTest, SingleShotStaticMethodMultiple)
{
    std::atomic<int> count{0};

    auto t1 = timer::single_shot(50, [&count]() { count++; });
    auto t2 = timer::single_shot(60, [&count]() { count++; });
    auto t3 = timer::single_shot(70, [&count]() { count++; });

    std::this_thread::sleep_for(100ms);

    EXPECT_EQ(count.load(), 3);
}

// ============================================================================
// Accuracy Mode Tests
// ============================================================================

TEST(TimerTest, AccuracyMode)
{
    timer t;

    EXPECT_EQ(t.accuracy(), timer_accuracy::precise);

    t.set_accuracy(timer_accuracy::coarse);
    EXPECT_EQ(t.accuracy(), timer_accuracy::coarse);

    t.set_accuracy(timer_accuracy::very_precise);
    EXPECT_EQ(t.accuracy(), timer_accuracy::very_precise);

    // Note: Accuracy mode doesn't affect behavior in current implementation
    // This just tests the API
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(TimerTest, ManyTimersSimultaneously)
{
    const int numTimers = 50;
    std::vector<std::unique_ptr<timer>> timers;
    std::atomic<int> totalCount{0};

    for (int i = 0; i < numTimers; ++i) {
        auto t = std::make_unique<timer>();
        t->timeout.connect([&totalCount]() {
            totalCount++;
        });
        t->start(10 + (i % 20));  // Varied intervals
        timers.push_back(std::move(t));
    }

    std::this_thread::sleep_for(100ms);

    // All timers should have fired at least once
    EXPECT_GT(totalCount.load(), numTimers);

    // Stop all timers
    timers.clear();
}

TEST(TimerTest, RapidStartStop)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    // Rapidly start and stop
    for (int i = 0; i < 100; ++i) {
        t.start(10);
        t.stop();
    }

    // Should not crash
    EXPECT_FALSE(t.is_active());
}

TEST(TimerTest, LongRunningTimer)
{
    timer t;
    std::atomic<int> count{0};

    t.timeout.connect([&count]() {
        count++;
    });

    // Run for extended period
    t.start(10);
    std::this_thread::sleep_for(500ms);

    int finalCount = count.load();
    EXPECT_GT(finalCount, 40);  // Should fire ~50 times

    t.stop();
}
