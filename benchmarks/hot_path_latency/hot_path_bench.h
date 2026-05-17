/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "hot_path_bench_config.h"

#include "quill/backend/BackendUtilities.h"
#include "quill/backend/RdtscClock.h"
#include "quill/core/Rdtsc.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

inline uint16_t get_cpu_to_pin_thread(uint16_t thread_num)
{
  auto const num_cores = static_cast<uint16_t>(std::thread::hardware_concurrency());

  // If hardware_concurrency feature is not supported, zero value is returned.
  if (num_cores == 0)
    return 0;

  return thread_num % num_cores;
}

// Instead of sleep
inline void wait([[maybe_unused]] std::chrono::nanoseconds min, std::chrono::nanoseconds max)
{
#ifdef PERF_ENABLED
  // when in perf use sleep as the other variables add noise
  std::this_thread::sleep_for(max);
#else
  thread_local std::mt19937 gen{std::random_device{}()};
  std::uniform_int_distribution<int64_t> dis(min.count(), max.count());

  auto const start_time = std::chrono::steady_clock::now();
  auto const end_time = start_time + std::chrono::nanoseconds{dis(gen)};
  std::chrono::nanoseconds time_now;
  do
  {
    time_now = std::chrono::steady_clock::now().time_since_epoch();
  } while (time_now < end_time.time_since_epoch());
#endif
}

inline size_t messages_for_thread(size_t messages_per_iteration, uint16_t thread_count, uint16_t thread_num) noexcept
{
  if (thread_count == 0)
  {
    return 0;
  }

  size_t const base_messages_per_thread = messages_per_iteration / thread_count;
  size_t const remainder = messages_per_iteration % thread_count;
  return base_messages_per_thread + (static_cast<size_t>(thread_num) < remainder ? 1u : 0u);
}

inline void wait_for_all_threads_to_start(std::atomic<size_t>& started_threads, size_t participant_count) noexcept
{
  started_threads.fetch_add(1, std::memory_order_acq_rel);
  while (started_threads.load(std::memory_order_acquire) != participant_count)
  {
  }
}

#ifdef PERF_ENABLED
/***/
inline void run_log_benchmark(size_t num_iterations, size_t messages_per_iteration,
                              std::function<void()> on_thread_start,
                              std::function<void(uint64_t, uint64_t, double)> log_func,
                              std::function<void()> on_thread_exit, std::atomic<size_t>& started_threads,
                              size_t participant_count, size_t current_thread_num)
{
  // running thread affinity
  quill::detail::set_cpu_affinity({get_cpu_to_pin_thread(static_cast<uint16_t>(current_thread_num))});

  on_thread_start();
  wait_for_all_threads_to_start(started_threads, participant_count);

  if (messages_per_iteration == 0)
  {
    on_thread_exit();
    return;
  }

  // Main Benchmark
  for (size_t iteration = 0; iteration < num_iterations; ++iteration)
  {
    double const d = iteration + (0.1 * iteration);

    for (size_t i = 0; i < messages_per_iteration; ++i)
    {
      log_func(iteration, i, d);
    }

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
                              std::function<void()> const& on_thread_exit, std::atomic<size_t>& started_threads,
                              size_t participant_count, uint16_t current_thread_num,
                              std::vector<uint64_t>& latencies, double rdtsc_ns_per_tick)
{
  // running thread affinity
  quill::detail::set_cpu_affinity({get_cpu_to_pin_thread(current_thread_num)});

  on_thread_start();
  wait_for_all_threads_to_start(started_threads, participant_count);

  if (messages_per_iteration == 0)
  {
    on_thread_exit();
    return;
  }

  // Main Benchmark
  for (size_t iteration = 0; iteration < num_iterations; ++iteration)
  {
    double const d = static_cast<double>(iteration) + (0.1 * static_cast<double>(iteration));

    auto const start = quill::detail::rdtsc();
    for (size_t i = 0; i < messages_per_iteration; ++i)
    {
      log_func(iteration, i, d);
    }
    auto const end = quill::detail::rdtsc();

    uint64_t const latency{static_cast<uint64_t>(
      static_cast<double>((end - start)) / static_cast<double>(messages_per_iteration) * rdtsc_ns_per_tick)};
    latencies.push_back(latency);

    // send the next batch of messages after x time
    wait(MIN_WAIT_DURATION, MAX_WAIT_DURATION);
  }

  on_thread_exit();
}
#endif

/***/
inline void run_benchmark([[maybe_unused]] char const* benchmark_name, uint16_t thread_count,
                          size_t num_iterations, [[maybe_unused]] size_t messages_per_iteration,
                          std::function<void()> const& on_thread_start,
                          std::function<void(uint64_t, uint64_t, double)> const& log_func,
                          std::function<void()> const& on_thread_exit)
{
  if (thread_count == 0)
  {
#ifndef PERF_ENABLED
    std::cout << "Thread Count 0 - Total messages 0 - " << benchmark_name << "\n | no latency samples |\n\n";
#endif
    return;
  }

  // main thread affinity
  quill::detail::set_cpu_affinity({0});

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
  std::atomic<size_t> started_threads{0};
  for (uint16_t thread_num = 0; thread_num < thread_count; ++thread_num)
  {
    size_t const thread_messages_per_iteration =
      messages_for_thread(messages_per_iteration, thread_count, thread_num);

#ifdef PERF_ENABLED
    // Spawn num threads
    threads.emplace_back(run_log_benchmark, num_iterations, thread_messages_per_iteration,
                         on_thread_start, log_func, on_thread_exit, std::ref(started_threads),
                         static_cast<size_t>(thread_count), thread_num + 1);
#else
    // Spawn num threads
    threads.emplace_back(run_log_benchmark, num_iterations, thread_messages_per_iteration,
                         std::ref(on_thread_start), std::ref(log_func), std::ref(on_thread_exit),
                         std::ref(started_threads), static_cast<size_t>(thread_count),
                         static_cast<uint16_t>(thread_num + 1u), std::ref(latencies[thread_num]),
                         rdtsc_clock.nanoseconds_per_tick());
#endif
  }

  // Wait for threads to finish
  for (uint16_t i = 0; i < thread_count; ++i)
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

  size_t const total_messages = num_iterations * messages_per_iteration;
  if (latencies_combined.empty())
  {
    std::cout << "Thread Count " << thread_count << " - Total messages " << total_messages << " - "
              << benchmark_name << "\n | no latency samples |\n\n";
    return;
  }

  size_t const sample_count = latencies_combined.size();
  auto percentile_index = [sample_count](double percentile)
  {
    size_t const nearest_rank =
      static_cast<size_t>(std::ceil(static_cast<double>(sample_count) * percentile));
    return std::min(sample_count - 1, nearest_rank == 0 ? size_t{0} : (nearest_rank - 1));
  };

  std::cout << "Thread Count " << thread_count << " - Total messages " << total_messages << " - "
            << benchmark_name << "\n |  50th | 75th | 90th | 95th | 99th | 99.9th | Worst |\n"
            << " |  " << latencies_combined[percentile_index(0.5)] << "  |  "
            << latencies_combined[percentile_index(0.75)] << "  |  "
            << latencies_combined[percentile_index(0.9)] << "  |  "
            << latencies_combined[percentile_index(0.95)] << "  |  "
            << latencies_combined[percentile_index(0.99)] << "  |  "
            << latencies_combined[percentile_index(0.999)] << "  |  "
            << latencies_combined[static_cast<size_t>(latencies_combined.size() - 1)] << "  |\n\n";
#endif
}
