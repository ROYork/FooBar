/// @file timer_examples.cpp
/// @brief Comprehensive examples for timer class
///
/// Demonstrates various timer usage patterns including:
/// - Basic repeating and single-shot timers
/// - Timer control (start, stop, restart)
/// - Signal/slot integration
/// - Cross-thread usage
/// - Practical applications

#include <fb/timer.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

using namespace fb;
using namespace std::chrono_literals;

// ============================================================================
// Example 1: Basic Repeating Timer
// ============================================================================

void example_basic_repeating_timer()
{
  std::cout << "\n=== Example 1: Basic Repeating Timer ===" << std::endl;

  timer t;
  int count = 0;

  // Connect a lambda to the timeout signal
  t.timeout.connect([&count]() {
    count++;
    std::cout << "Timer fired! Count: " << count << std::endl;
  });

  // Start timer with 500ms interval
  t.start(500);

  std::cout << "Timer started, interval: " << t.interval() << "ms" << std::endl;

  // Run for 2.5 seconds
  std::this_thread::sleep_for(2500ms);

  t.stop();
  std::cout << "Timer stopped. Total fires: " << count << std::endl;
}

// ============================================================================
// Example 2: Single-Shot Timer
// ============================================================================

void example_single_shot_timer()
{
  std::cout << "\n=== Example 2: Single-Shot Timer ===" << std::endl;

  timer t;

  t.set_single_shot(true);
  t.timeout.connect([]() {
    std::cout << "Single-shot timer fired!" << std::endl;
  });

  t.start(1000);

  std::cout << "Waiting for single-shot timeout..." << std::endl;
  std::this_thread::sleep_for(1500ms);

  std::cout << "Timer active: " << (t.is_active() ? "yes" : "no") << std::endl;
}

// ============================================================================
// Example 3: Using timer::single_shot() Static Method
// ============================================================================

void example_static_single_shot()
{
  std::cout << "\n=== Example 3: Static single_shot() Method ===" << std::endl;

  std::cout << "Starting 3-second countdown..." << std::endl;

  auto t = timer::single_shot(3000, []() {
    std::cout << "3 seconds elapsed!" << std::endl;
  });

  // Keep reference alive
  std::this_thread::sleep_for(3500ms);

  std::cout << "Done" << std::endl;
}

// ============================================================================
// Example 4: Timer Control Operations
// ============================================================================

void example_timer_control()
{
  std::cout << "\n=== Example 4: Timer Control (start/stop/restart) ===" << std::endl;

  timer t;
  std::atomic<int> count{0};

  t.timeout.connect([&count]() {
    count++;
    std::cout << "  Tick " << count << std::endl;
  });

  std::cout << "Starting timer (300ms interval)..." << std::endl;
  t.start(300);
  std::this_thread::sleep_for(1000ms);

  std::cout << "Stopping timer..." << std::endl;
  t.stop();
  const int count_after_stop = count.load();
  std::this_thread::sleep_for(500ms);

  std::cout << "Count after stop: " << count_after_stop << " (should not change)" << std::endl;

  std::cout << "Restarting timer..." << std::endl;
  count = 0;
  t.restart();
  std::this_thread::sleep_for(1000ms);

  t.stop();
  std::cout << "Final count: " << count << std::endl;
}

// ============================================================================
// Example 5: Remaining Time Query
// ============================================================================

void example_remaining_time()
{
  std::cout << "\n=== Example 5: Remaining Time Query ===" << std::endl;

  timer t;

  t.set_single_shot(true);
  t.timeout.connect([]() {
    std::cout << "Timer expired!" << std::endl;
  });

  t.start(5000);

  for (int i = 0; i < 6; ++i)
  {
    int remaining = t.remaining_time();
    std::cout << "Remaining: " << remaining << "ms" << std::endl;
    std::this_thread::sleep_for(1000ms);
  }
}

// ============================================================================
// Example 6: Multiple Timers
// ============================================================================

void example_multiple_timers()
{
  std::cout << "\n=== Example 6: Multiple Timers ===" << std::endl;

  timer fast_timer;
  timer medium_timer;
  timer slow_timer;

  std::atomic<int> fast_count{0};
  std::atomic<int> medium_count{0};
  std::atomic<int> slow_count{0};

  fast_timer.timeout.connect([&fast_count]() {
    fast_count++;
    std::cout << "  Fast   tick " << fast_count << std::endl;
  });

  medium_timer.timeout.connect([&medium_count]() {
    medium_count++;
    std::cout << "  Medium tick " << medium_count << std::endl;
  });

  slow_timer.timeout.connect([&slow_count]() {
    slow_count++;
    std::cout << "  Slow   tick " << slow_count << std::endl;
  });

  fast_timer.start(200);
  medium_timer.start(500);
  slow_timer.start(1000);

  std::this_thread::sleep_for(3000ms);

  fast_timer.stop();
  medium_timer.stop();
  slow_timer.stop();

  std::cout << "\nFinal counts - Fast: " << fast_count
            << ", Medium: " << medium_count
            << ", Slow: " << slow_count << std::endl;
}

// ============================================================================
// Example 7: Countdown Timer
// ============================================================================

void example_countdown_timer()
{
  std::cout << "\n=== Example 7: Countdown Timer ===" << std::endl;

  int seconds = 10;
  timer t;

  t.timeout.connect([&seconds, &t]() {
    std::cout << "\r" << seconds << " seconds remaining...   " << std::flush;
    seconds--;

    if (seconds < 0)
    {
      t.stop();
      std::cout << "\rCountdown complete!            " << std::endl;
    }
  });

  t.start(1000);

  // Wait for countdown to complete
  while (t.is_active())
  {
    std::this_thread::sleep_for(100ms);
  }
}

// ============================================================================
// Example 8: Stopwatch with Lap Times
// ============================================================================

void example_stopwatch()
{
  std::cout << "\n=== Example 8: Stopwatch with Lap Times ===" << std::endl;

  timer t;
  auto start_time = std::chrono::steady_clock::now();
  std::vector<std::chrono::milliseconds> lap_times;

  // Update every 100ms
  t.timeout.connect([&start_time, &lap_times]() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);

    std::cout << "\rElapsed: " << elapsed.count() << "ms   " << std::flush;

    // Record lap every second
    if (elapsed.count() % 1000 < 100)
    {
      lap_times.push_back(elapsed);
    }
  });

  t.start(100);

  // Run for 5 seconds
  std::this_thread::sleep_for(5000ms);

  t.stop();

  std::cout << "\n\nLap Times:" << std::endl;
  for (size_t i = 0; i < lap_times.size(); ++i)
  {
    std::cout << "  Lap " << (i + 1) << ": " << lap_times[i].count() << "ms" << std::endl;
  }
}

// ============================================================================
// Example 9: Periodic Data Collection
// ============================================================================

class DataCollector
{
public:
  DataCollector(int interval_ms)
  {
    m_timer.timeout.connect([this]() {
      collect_data();
    });
    m_timer.start(interval_ms);
  }

  ~DataCollector()
  {
    m_timer.stop();
  }

  void collect_data()
  {
    // Simulate collecting data
    double value = 20.0 + (rand() % 100) / 10.0;
    m_samples.push_back(value);

    std::cout << "Collected sample " << m_samples.size()
              << ": " << std::fixed << std::setprecision(1)
              << value << "°C" << std::endl;
  }

  double get_average() const
  {
    if (m_samples.empty()) return 0.0;

    double sum = 0.0;
    for (double s : m_samples)
    {
      sum += s;
    }
    return sum / static_cast<double>(m_samples.size());
  }

  size_t get_sample_count() const { return m_samples.size(); }

private:
  timer m_timer;
  std::vector<double> m_samples;
};

void example_data_collection()
{
  std::cout << "\n=== Example 9: Periodic Data Collection ===" << std::endl;

  DataCollector collector(500);  // Collect every 500ms

  std::cout << "Collecting data..." << std::endl;
  std::this_thread::sleep_for(3000ms);

  std::cout << "\nResults:" << std::endl;
  std::cout << "  Samples collected: " << collector.get_sample_count() << std::endl;
  std::cout << "  Average: " << std::fixed << std::setprecision(2)
            << collector.get_average() << "°C" << std::endl;
}

// ============================================================================
// Example 10: Delayed Action
// ============================================================================

void example_delayed_action()
{
  std::cout << "\n=== Example 10: Delayed Action ===" << std::endl;

  std::cout << "Scheduling delayed actions..." << std::endl;

  // Action after 1 second
  auto t1 = timer::single_shot(1000, []() {
    std::cout << "  Action 1: 1 second elapsed" << std::endl;
  });

  // Action after 2 seconds
  auto t2 = timer::single_shot(2000, []() {
    std::cout << "  Action 2: 2 seconds elapsed" << std::endl;
  });

  // Action after 3 seconds
  auto t3 = timer::single_shot(3000, []() {
    std::cout << "  Action 3: 3 seconds elapsed" << std::endl;
  });

  std::this_thread::sleep_for(3500ms);
  std::cout << "All actions completed" << std::endl;
}

// ============================================================================
// Example 11: Timeout Detection
// ============================================================================

void example_timeout_detection()
{
  std::cout << "\n=== Example 11: Timeout Detection ===" << std::endl;

  std::atomic<bool> operation_complete{false};
  std::atomic<bool> timed_out{false};

  timer watchdog;
  watchdog.set_single_shot(true);
  watchdog.timeout.connect([&timed_out]() {
    timed_out = true;
    std::cout << "Operation timed out!" << std::endl;
  });

  // Start 2-second watchdog
  watchdog.start(2000);

  // Simulate long-running operation
  std::cout << "Starting operation (takes 3 seconds)..." << std::endl;
  std::thread worker([&operation_complete]() {
    std::this_thread::sleep_for(3000ms);
    operation_complete = true;
  });

  worker.join();

  if (timed_out)
  {
    std::cout << "Operation exceeded timeout limit" << std::endl;
  }
  else
  {
    std::cout << "Operation completed within timeout" << std::endl;
    watchdog.stop();
  }
}

// ============================================================================
// Example 12: Heartbeat Monitor
// ============================================================================

class HeartbeatMonitor
{
public:
  HeartbeatMonitor(int heartbeat_interval_ms, int timeout_ms)
      : m_heartbeat_interval(heartbeat_interval_ms)
  {
    // Setup heartbeat timer
    m_heartbeat_timer.timeout.connect([this]() {
      send_heartbeat();
    });

    // Setup timeout watchdog
    m_watchdog_timer.set_single_shot(true);
    m_watchdog_timer.timeout.connect([this]() {
      on_timeout();
    });

    m_heartbeat_timer.start(m_heartbeat_interval);
    m_watchdog_timer.start(timeout_ms);
  }

  void receive_ack()
  {
    std::cout << "  ACK received" << std::endl;
    m_watchdog_timer.restart();
  }

  ~HeartbeatMonitor()
  {
    m_heartbeat_timer.stop();
    m_watchdog_timer.stop();
  }

private:
  void send_heartbeat()
  {
    std::cout << "Sending heartbeat..." << std::endl;
  }

  void on_timeout()
  {
    std::cout << "*** Connection timeout - no ACK received ***" << std::endl;
    m_heartbeat_timer.stop();
  }

  timer m_heartbeat_timer;
  timer m_watchdog_timer;
  int m_heartbeat_interval;
};

void example_heartbeat_monitor()
{
  std::cout << "\n=== Example 12: Heartbeat Monitor ===" << std::endl;

  HeartbeatMonitor monitor(1000, 3000);  // Heartbeat every 1s, timeout after 3s

  // Simulate receiving ACKs
  std::this_thread::sleep_for(1500ms);
  monitor.receive_ack();

  std::this_thread::sleep_for(1500ms);
  monitor.receive_ack();

  // Stop receiving ACKs - should timeout
  std::this_thread::sleep_for(4000ms);

  std::cout << "Monitor test complete" << std::endl;
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  std::cout << R"(
╔══════════════════════════════════════════════════════════════════════════════╗
║                                                                              ║
║                     timer class - Comprehensive Examples                     ║
║                                                                              ║
║  Demonstrates timer functionality with fb_signal integration                ║
║                                                                              ║
╚══════════════════════════════════════════════════════════════════════════════╝
)" << std::endl;

  try
  {
    example_basic_repeating_timer();
    example_single_shot_timer();
    example_static_single_shot();
    example_timer_control();
    example_remaining_time();
    example_multiple_timers();
    example_countdown_timer();
    example_stopwatch();
    example_data_collection();
    example_delayed_action();
    example_timeout_detection();
    example_heartbeat_monitor();

    std::cout << "\n=== All Examples Complete ===" << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
