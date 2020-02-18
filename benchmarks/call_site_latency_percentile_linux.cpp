#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>

#include "quill/Quill.h"

/* Returns nanoseconds since epoch */
QUILL_ALWAYS_INLINE uint64_t timestamp_now()
{
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

template <typename Function>
void run_log_benchmark(Function&& f, char const* benchmark_name, std::mutex& m, int thread_num)
{
  // Always ignore the first log statement as it will be doing initialisation for most loggers
  f(100, 100, "initial");

  int iterations = 100000;
  std::vector<uint64_t> latencies;
  constexpr char const* str = "benchmark";

  for (int i = 0; i < iterations; ++i)
  {
    // generate a double from i
    double const d = i + (0.1 * i);
    uint64_t const latency = f(i, d, str);
    latencies.push_back(latency);
  }

  // Sort all latencies
  std::sort(latencies.begin(), latencies.end());

  // Calculate the sum of all latencies
  uint64_t sum = std::accumulate(latencies.begin(), latencies.end(), static_cast<uint64_t>(0));

  // protect access to cout
  std::lock_guard lock{m};
  std::cout << "Thread: " << thread_num << std::setw(12) << "50th" << std::setw(20) << "75th"
            << std::setw(20) << "90th" << std::setw(19) << "99th" << std::setw(20) << "99.9th"
            << std::setw(20) << "Worst" << std::setw(21) << "Average\n"
            << std::setw(20) << latencies[(size_t)iterations * 0.5] << std::setw(20)
            << latencies[(size_t)iterations * 0.75] << std::setw(20) << latencies[(size_t)iterations * 0.9]
            << std::setw(20) << latencies[(size_t)iterations * 0.99] << std::setw(20)
            << latencies[(size_t)iterations * 0.999] << std::setw(20) << latencies[latencies.size() - 1]
            << std::setw(20) << (sum * 1.0) / latencies.size() << "\n\n";
}

template <typename Function>
void run_benchmark(Function&& f, int32_t thread_count, char const* benchmark_name)
{
  std::cout << "********************************* " << std::endl;
  std::cout << "Total thread count: " << thread_count << " - " << benchmark_name
            << " in nanoseconds " << std::endl;

  std::mutex m;
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i)
  {
    // Spawn num threads
    threads.emplace_back(run_log_benchmark<Function>, std::ref(f), benchmark_name, std::ref(m), i + 1);
  }

  // Wait for threads to finish
  for (int i = 0; i < thread_count; ++i)
  {
    threads[i].join();
  }
}

int main(int argc, char* argv[])
{

  if (argc != 2)
  {
    std::cerr << "Please provide the name of the logger as argument." << std::endl;
    return 0;
  }

  if (strcmp(argv[1], "quill") == 0)
  {
    // Setup

    // Set the main thread affinity to cpu 0
    quill::detail::set_cpu_affinity(0);

    // Pin the backend thread to cpu 1
    quill::config::set_backend_thread_cpu_affinity(1);
    quill::config::set_backend_thread_sleep_duration(std::chrono::nanoseconds{0});

    // Start the logging backend thread
    quill::start();

    // wait for the backend thread to start
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Create a file handler to write to a file
    quill::Handler* file_handler =
      quill::file_handler("quill_call_site_latency_percentile_linux_benchmark", "w");

    quill::Logger* logger = quill::create_logger("bench_logger", file_handler);

    // Define a logging lambda
    auto quill_benchmark = [logger](int32_t i, double d, char const* str) {
      uint64_t const start = timestamp_now();
      LOG_INFO(logger, "Logging str: {}, int: {}, double: {}", str, i, d);
      uint64_t const end = timestamp_now();
      return end - start;
    };

    std::array<int32_t, 4> threads_num{{1, 2, 3, 4}};
    // Run the benchmark for n threads
    for (auto threads : threads_num)
    {
      run_benchmark(quill_benchmark, threads, "Logger: Quill - Benchmark: Caller Thread Latency");
    }
  }

  return 0;
}