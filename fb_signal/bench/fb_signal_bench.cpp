/// @file fb_signal_bench.cpp
/// @brief Standardized benchmark suite for signal/slot library comparison
/// @note Multi-threaded scenarios for fair performance evaluation


#include <fb/event_queue.hpp>
#include <fb/signal.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

namespace {

using clock_type = std::chrono::steady_clock;
using ns_duration = std::chrono::nanoseconds;

constexpr std::size_t WARMUP_ITERS = 10000;
constexpr std::size_t BENCH_ITERS = 1000000;
constexpr std::size_t SAMPLE_COUNT = 1000;

// ============================================================================
// Timing utilities
// ============================================================================

struct stats {
  double min_ns;
  double max_ns;
  double mean_ns;
  double median_ns;
  double p99_ns;

  static stats compute(std::vector<double> &samples) {
    stats s{};
    if (samples.empty())
      return s;

    std::sort(samples.begin(), samples.end());
    s.min_ns = samples.front();
    s.max_ns = samples.back();
    s.mean_ns = std::accumulate(samples.begin(), samples.end(), 0.0) /
                static_cast<double>(samples.size());

    std::size_t mid = samples.size() / 2;
    s.median_ns = (samples.size() % 2 == 0)
                      ? (samples[mid - 1] + samples[mid]) / 2.0
                      : samples[mid];

    std::size_t p99_idx = static_cast<std::size_t>(samples.size() * 0.99);
    if (p99_idx >= samples.size())
      p99_idx = samples.size() - 1;
    s.p99_ns = samples[p99_idx];

    return s;
  }
};

template <typename Fn>
stats benchmark_latency(Fn &&fn, std::size_t warmup, std::size_t samples,
                        std::size_t ops_per_sample) {
  // Warmup
  for (std::size_t i = 0; i < warmup; ++i)
    fn();

  // Collect samples
  std::vector<double> timings;
  timings.reserve(samples);

  for (std::size_t s = 0; s < samples; ++s) {
    auto start = clock_type::now();
    for (std::size_t i = 0; i < ops_per_sample; ++i)
      fn();
    auto end = clock_type::now();

    double ns = std::chrono::duration<double, std::nano>(end - start).count();
    timings.push_back(ns / static_cast<double>(ops_per_sample));
  }

  return stats::compute(timings);
}

void print_result(const char *name, const stats &s) {
  std::cout << std::left << std::setw(40) << name << " median: " << std::setw(8)
            << std::fixed << std::setprecision(1) << s.median_ns << " ns"
            << "  mean: " << std::setw(8) << s.mean_ns << " ns"
            << "  p99: " << std::setw(8) << s.p99_ns << " ns\n";
}

// ============================================================================
// Benchmark implementations  
// ============================================================================

void bench_emit_1_slot() {
  fb::signal<int> sig;
  volatile int sink = 0;

  auto conn = sig.connect([&sink](int v) { sink += v; });

  auto s =
      benchmark_latency([&] { sig.emit(1); }, WARMUP_ITERS, SAMPLE_COUNT, 1000);
  print_result("emit_1_slot (direct)", s);
}

void bench_emit_10_slots() {
  fb::signal<int> sig;
  volatile int sink = 0;

  std::vector<fb::connection> conns;
  for (int i = 0; i < 10; ++i) {
    conns.push_back(sig.connect([&sink](int v) { sink += v; }));
  }

  auto s =
      benchmark_latency([&] { sig.emit(1); }, WARMUP_ITERS, SAMPLE_COUNT, 1000);
  print_result("emit_10_slots (direct)", s);
}

void bench_emit_100_slots() {
  fb::signal<int> sig;
  volatile int sink = 0;

  std::vector<fb::connection> conns;
  for (int i = 0; i < 100; ++i) {
    conns.push_back(sig.connect([&sink](int v) { sink += v; }));
  }

  auto s =
      benchmark_latency([&] { sig.emit(1); }, WARMUP_ITERS, SAMPLE_COUNT, 100);
  print_result("emit_100_slots (direct)", s);
}

void bench_concurrent_emit(int num_threads) {
  fb::signal<int> sig;
  std::atomic<int64_t> sink{0};

  auto conn = sig.connect(
      [&sink](int v) { sink.fetch_add(v, std::memory_order_relaxed); });

  constexpr std::size_t ITERS_PER_THREAD = 100000;
  std::atomic<bool> go{false};

  std::vector<std::thread> threads;
  std::vector<double> thread_times(num_threads);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      while (!go.load(std::memory_order_acquire)) {
      }

      auto start = clock_type::now();
      for (std::size_t i = 0; i < ITERS_PER_THREAD; ++i) {
        sig.emit(1);
      }
      auto end = clock_type::now();
      thread_times[t] =
          std::chrono::duration<double, std::nano>(end - start).count() /
          ITERS_PER_THREAD;
    });
  }

  go.store(true, std::memory_order_release);
  for (auto &t : threads)
    t.join();

  auto s = stats::compute(thread_times);
  std::string name =
      "concurrent_emit_" + std::to_string(num_threads) + "_threads";
  print_result(name.c_str(), s);
}

void bench_emit_with_churn() {
  fb::signal<int> sig;
  std::atomic<int64_t> sink{0};
  std::atomic<bool> running{true};

  // Stable connection
  auto stable = sig.connect(
      [&sink](int v) { sink.fetch_add(v, std::memory_order_relaxed); });

  // Churn thread: constantly connects/disconnects
  std::thread churn_thread([&]() {
    while (running.load(std::memory_order_relaxed)) {
      auto c = sig.connect(
          [&sink](int v) { sink.fetch_add(v, std::memory_order_relaxed); });
      c.disconnect();
    }
  });

  // Benchmark emit under churn
  auto s =
      benchmark_latency([&] { sig.emit(1); }, WARMUP_ITERS, SAMPLE_COUNT, 100);

  running.store(false);
  churn_thread.join();

  print_result("emit_with_connection_churn", s);
}

void bench_connect_disconnect() {
  fb::signal<int> sig;
  volatile int sink = 0;

  auto s = benchmark_latency(
      [&] {
        auto c = sig.connect([&sink](int v) { sink += v; });
        c.disconnect();
      },
      1000, SAMPLE_COUNT, 100);

  print_result("connect_disconnect_cycle", s);
}

void bench_bulk_connect() {
  constexpr int BULK_SIZE = 1000;
  volatile int sink = 0;

  std::vector<double> timings;
  timings.reserve(100);

  for (int trial = 0; trial < 100; ++trial) {
    fb::signal<int> sig;
    std::vector<fb::connection> conns;
    conns.reserve(BULK_SIZE);

    auto start = clock_type::now();
    for (int i = 0; i < BULK_SIZE; ++i) {
      conns.push_back(sig.connect([&sink](int v) { sink += v; }));
    }
    auto end = clock_type::now();

    double ns = std::chrono::duration<double, std::nano>(end - start).count() /
                BULK_SIZE;
    timings.push_back(ns);
  }

  auto s = stats::compute(timings);
  print_result("bulk_connect_1000 (per connection)", s);
}

void bench_queued_emit() {
  fb::signal<int> sig;
  fb::event_queue queue;
  volatile int sink = 0;

  auto conn = sig.connect([&sink](int v) { sink += v; },
                          fb::delivery_policy::queued, queue);

  // Warmup and drain
  for (std::size_t i = 0; i < WARMUP_ITERS; ++i)
    sig.emit(1);
  queue.process_pending();

  // Benchmark enqueue time only
  std::vector<double> timings;
  timings.reserve(SAMPLE_COUNT);

  for (std::size_t s = 0; s < SAMPLE_COUNT; ++s) {
    auto start = clock_type::now();
    for (int i = 0; i < 100; ++i)
      sig.emit(1);
    auto end = clock_type::now();

    double ns =
        std::chrono::duration<double, std::nano>(end - start).count() / 100;
    timings.push_back(ns);
    queue.process_pending();
  }

  auto st = stats::compute(timings);
  print_result("queued_emit_enqueue", st);
}

} // namespace

int main() {
  std::cout << "=== FooBar Benchmark Results ===\n\n";
  std::cout << "--- Hot Path Emission ---\n";
  bench_emit_1_slot();
  bench_emit_10_slots();
  bench_emit_100_slots();

  std::cout << "\n--- Multi-Threaded Contention ---\n";
  bench_concurrent_emit(4);
  bench_concurrent_emit(8);
  bench_emit_with_churn();

  std::cout << "\n--- Connection Management (Cold Path) ---\n";
  bench_connect_disconnect();
  bench_bulk_connect();

  std::cout << "\n--- Queued Delivery ---\n";
  bench_queued_emit();

  std::cout << "\n=== Benchmark Complete ===\n";
  return 0;
}
