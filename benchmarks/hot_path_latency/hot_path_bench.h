/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "hot_path_bench_config.h"
#include "quill/detail/misc/Os.h"
#include "quill/detail/misc/Rdtsc.h"
#include "quill/detail/misc/RdtscClock.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

#if defined(_WIN32)
  #include <intrin.h>
#else
  #include <x86intrin.h>
#endif

inline unsigned get_cpu_to_pin_thread(size_t thread_num)
{
  const unsigned num_cores = std::thread::hardware_concurrency();

  // If hardware_concurrency feature is not supported, zero value is returned.
  if (num_cores == 0)
    return 0;

  return thread_num % num_cores;
}

// Instead of sleep
inline void wait(std::chrono::nanoseconds min, std::chrono::nanoseconds max)
{
#ifdef PERF_ENABLED
  // when in perf use sleep as the other variables add noise
  std::this_thread::sleep_for(max);
#else
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(static_cast<int>(min.count()), static_cast<int>(max.count()));

  auto const start_time = std::chrono::steady_clock::now();
  auto const end_time = start_time.time_since_epoch() + std::chrono::nanoseconds{dis(gen)};
  std::chrono::nanoseconds time_now;
  do
  {
    time_now = std::chrono::steady_clock::now().time_since_epoch();
  } while (time_now < end_time);
#endif
}

#ifdef PERF_ENABLED
/***/
inline void run_log_benchmark(size_t num_iterations, size_t messages_per_iteration,
                              std::function<void()> on_thread_start,
                              std::function<void(uint64_t, uint64_t, double)> log_func,
                              std::function<void()> on_thread_exit, size_t current_thread_num)
{
  // running thread affinity
  quill::detail::set_cpu_affinity(get_cpu_to_pin_thread(current_thread_num));

  on_thread_start();

  unsigned int aux;
  // Main Benchmark
  for (size_t iteration = 0; iteration < num_iterations; ++iteration)
  {
    double const d = iteration + (0.1 * iteration);

    auto const start = __rdtscp(&aux);
    for (size_t i = 0; i < messages_per_iteration; ++i)
    {
      log_func(iteration, i, d);
    }
    auto const end = __rdtscp(&aux);

    // send the next batch of messages after x time
    wait(MIN_WAIT_DURATION, MAX_WAIT_DURATION);
  }

  on_thread_exit();
}
#else
/***/
inline void run_log_benchmark(size_t num_iterations, size_t messages_per_iteration,
                              std::function<void()> const& on_thread_start,
                              std::function<void(uint64_t, uint64_t, double)> const& log_func,
                              std::function<void()> const& on_thread_exit, uint16_t current_thread_num,
                              std::vector<uint64_t>& latencies, double rdtsc_ns_per_tick)
{
  // running thread affinity
  quill::detail::set_cpu_affinity(get_cpu_to_pin_thread(current_thread_num));

  on_thread_start();

  unsigned int aux;
  // Main Benchmark
  for (size_t iteration = 0; iteration < num_iterations; ++iteration)
  {
    double const d = iteration + (0.1 * iteration);

    auto const start = __rdtscp(&aux);
    for (size_t i = 0; i < messages_per_iteration; ++i)
    {
      log_func(iteration, i, d);
    }
    auto const end = __rdtscp(&aux);

    uint64_t const latency{static_cast<uint64_t>((end - start) / messages_per_iteration * rdtsc_ns_per_tick)};
    latencies.push_back(latency);

    // send the next batch of messages after x time
    wait(MIN_WAIT_DURATION, MAX_WAIT_DURATION);
  }

  on_thread_exit();
}
#endif

/***/
inline void run_benchmark(char const* benchmark_name, int32_t thread_count, size_t num_iterations,
                          size_t messages_per_iteration, std::function<void()> const& on_thread_start,
                          std::function<void(uint64_t, uint64_t, double)> const& log_func,
                          std::function<void()> const& on_thread_exit)
{
  // main thread affinity
  quill::detail::set_cpu_affinity(0);

#ifndef PERF_ENABLED
  std::cout << "running for " << thread_count << " thread(s)" << std::endl;

  quill::detail::RdtscClock rdtsc_clock{std::chrono::minutes{30}};

  // each thread gets a vector of latencies
  std::vector<std::vector<uint64_t>> latencies;
  latencies.resize(thread_count);
  for (auto& elem : latencies)
  {
    elem.reserve(num_iterations);
  }
#endif

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  for (int thread_num = 0; thread_num < thread_count; ++thread_num)
  {
#ifdef PERF_ENABLED
    // Spawn num threads
    threads.emplace_back(run_log_benchmark, num_iterations, (messages_per_iteration / thread_count),
                         on_thread_start, log_func, on_thread_exit, thread_num + 1);
#else
    // Spawn num threads
    threads.emplace_back(run_log_benchmark, num_iterations, (messages_per_iteration / thread_count),
                         std::ref(on_thread_start), std::ref(log_func), std::ref(on_thread_exit),
                         thread_num + 1, std::ref(latencies[thread_num]), rdtsc_clock.nanoseconds_per_tick());
#endif
  }

  // Wait for threads to finish
  for (int i = 0; i < thread_count; ++i)
  {
    threads[i].join();
  }

#ifndef PERF_ENABLED
  // All threads have finished we can read all latencies
  std::vector<uint64_t> latencies_combined;
  latencies_combined.reserve(num_iterations * thread_count);
  for (auto const& elem : latencies)
  {
    latencies_combined.insert(latencies_combined.end(), elem.begin(), elem.end());
  }

  // Sort all latencies
  std::sort(latencies_combined.begin(), latencies_combined.end());

  std::cout << "Thread Count " << thread_count << " - Total messages "
            << latencies_combined.size() * messages_per_iteration << " - " << benchmark_name
            << "\n |  50th | 75th | 90th | 95th | 99th | 99.9th | Worst |\n"
            << " |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.5)]
            << "  |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.75)]
            << "  |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.9)]
            << "  |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.95)]
            << "  |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.99)]
            << "  |  " << latencies_combined[(size_t)((size_t)(num_iterations * thread_count) * 0.999)]
            << "  |  " << latencies_combined[latencies_combined.size() - 1] << "  |\n\n";
#endif
}