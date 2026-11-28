/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#include "hot_path_bench.h"

#include <cstdint>
#include <vector>

/**
 * The measurement floor: the same harness, schedule and payloads as the Quill benchmarks, with a
 * call that only stores its arguments, to a volatile on the calling thread's own stack, instead of
 * logging them. Its numbers are the cost of timing a sample, which is part of every other
 * benchmark's numbers too. Each thread stores to its own stack, so the threads share no cache line.
 */
void timer_only_benchmark(std::vector<int32_t> const& thread_count_array, size_t num_iterations_per_thread)
{
#if defined(BENCH_DISABLED_EXPENSIVE_ARG)
  // The reference of the disabled argument scenario: the argument is computed on every call
  auto log_func = [](uint64_t i, uint64_t j, double d)
  {
    [[maybe_unused]] uintptr_t volatile sink =
      static_cast<uintptr_t>(i + j) + static_cast<uintptr_t>(bench_expensive_argument(d));
  };
#else
  auto log_func = [](uint64_t i, uint64_t j, auto const& v)
  {
    [[maybe_unused]] uintptr_t volatile sink =
      static_cast<uintptr_t>(i + j) + reinterpret_cast<uintptr_t>(&v);
  };
#endif

  auto on_start = []() {};
  auto on_exit = []() {};

  for (auto thread_count : thread_count_array)
  {
    run_benchmark("Logger: Timer only - Benchmark: Caller Thread Latency", thread_count,
                  num_iterations_per_thread, MESSAGES_PER_ITERATION, on_start, log_func, on_exit);
  }
}

/***/
int main() { timer_only_benchmark(THREAD_LIST_COUNT, ITERATIONS); }
