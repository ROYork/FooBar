#include <fb/string_utils.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

namespace
{

/**
 * @brief Naive split implementation for comparison
 */
std::vector<std::string> naive_split(const std::string& str, char delimiter)
{
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delimiter))
  {
    result.push_back(token);
  }
  return result;
}

/**
 * @brief Generate a random string with delimiters
 */
std::string generate_test_string(size_t num_parts, size_t part_length)
{
  static const char charset[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<size_t> dist(
      0,
      sizeof(charset) - 2);

  std::string result;
  result.reserve(num_parts * (part_length + 1));

  for (size_t i = 0; i < num_parts; ++i)
  {
    if (i > 0)
    {
      result += ',';
    }
    for (size_t j = 0; j < part_length; ++j)
    {
      result += charset[dist(gen)];
    }
  }
  return result;
}

template<typename Func>
double benchmark(Func func, int iterations)
{
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i)
  {
    func();
  }
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count() /
         static_cast<double>(iterations);
}

void run_benchmark(const std::string& name,
                   size_t num_parts,
                   size_t part_length,
                   int iterations)
{
  std::string test_str = generate_test_string(num_parts, part_length);

  std::cout << "\n" << name << "\n";
  std::cout << "  String size: " << test_str.size() << " bytes\n";
  std::cout << "  Parts: " << num_parts << ", Part length: " << part_length
            << "\n";
  std::cout << "  Iterations: " << iterations << "\n\n";

  // Benchmark fb::split
  double fb_time = benchmark([&]() { fb::split(test_str, ','); }, iterations);

  // Benchmark naive split
  double naive_time = benchmark([&]() { naive_split(test_str, ','); }, iterations);

  std::cout << std::fixed << std::setprecision(4);
  std::cout << "  fb::split:    " << std::setw(10) << fb_time << " ms/iter\n";
  std::cout << "  naive_split:  " << std::setw(10) << naive_time << " ms/iter\n";
  std::cout << "  Speedup:      " << std::setw(10) << (naive_time / fb_time)
            << "x\n";
}

void run_string_benchmark(const std::string& name,
                          const std::string& test_str,
                          int iterations)
{
  std::cout << "\n" << name << " (iterations: " << iterations << ")\n";
  std::cout << std::string(60, '-') << "\n";

  // trim benchmark
  double time = benchmark([&]() {
    std::string padded = "   " + test_str + "   ";
    fb::trim(padded);
  }, iterations);
  std::cout << "  trim:           " << std::fixed << std::setprecision(4)
            << std::setw(10) << time << " ms/iter\n";

  // to_upper benchmark
  time = benchmark([&]() { fb::to_upper(test_str); }, iterations);
  std::cout << "  to_upper:       " << std::setw(10) << time << " ms/iter\n";

  // replace_all benchmark
  time = benchmark([&]() { fb::replace_all(test_str, "a", "x"); }, iterations);
  std::cout << "  replace_all:    " << std::setw(10) << time << " ms/iter\n";

  // contains benchmark
  time = benchmark([&]() { fb::contains(test_str, "xyz"); }, iterations);
  std::cout << "  contains:       " << std::setw(10) << time << " ms/iter\n";

  // count benchmark
  time = benchmark([&]() { fb::count(test_str, "a"); }, iterations);
  std::cout << "  count:          " << std::setw(10) << time << " ms/iter\n";
}

} // namespace

int main()
{
  std::cout << "=================================================\n";
  std::cout << "         fb_strings Performance Benchmarks       \n";
  std::cout << "=================================================\n";

  // Split benchmarks
  std::cout << "\n--- Split Benchmarks ---\n";
  run_benchmark("Small strings (10 parts, 10 chars each)", 10, 10, 100000);
  run_benchmark("Medium strings (100 parts, 50 chars each)", 100, 50, 10000);
  run_benchmark("Large strings (1000 parts, 100 chars each)", 1000, 100, 1000);

  // General string operation benchmarks
  std::cout << "\n--- General String Benchmarks ---\n";

  std::string small_str = generate_test_string(10, 10);
  std::string medium_str = generate_test_string(100, 10);
  std::string large_str = generate_test_string(1000, 10);

  run_string_benchmark("Small string (~100 bytes)", small_str, 100000);
  run_string_benchmark("Medium string (~1KB)", medium_str, 10000);
  run_string_benchmark("Large string (~10KB)", large_str, 1000);

  std::cout << "\n=================================================\n";
  std::cout << "                 Benchmarks Complete              \n";
  std::cout << "=================================================\n";

  return 0;
}
