#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <thread>

#include "quill/Quill.h"

template <typename Function1, typename Function2>
void run_log_benchmark(Function1&& f1, Function2&& f2, char const* benchmark_name, int thread_num, uint32_t iterations)
{
  // Set the caller thread affinity to a different cpu, cpu 0 is used by the backend
  quill::detail::set_cpu_affinity(thread_num + 1);

  constexpr char const* str = "benchmark";

  for (int i = 0; i < iterations; ++i)
  {
    // generate a double from i
    double const d = i + (0.1 * i);
    f1(i, d, str);
  }

  // on exit
  f2();
}

template <typename Function1, typename Function2>
void run_benchmark(Function1&& f1, Function2&& f2, int32_t thread_count, char const* benchmark_name, uint32_t iterations)
{
  std::vector<std::thread> threads;
  threads.reserve(thread_count);

  auto const start = std::chrono::steady_clock::now();

  for (int i = 0; i < thread_count; ++i)
  {
    // Spawn num threads
    threads.emplace_back(run_log_benchmark<Function1, Function2>, std::ref(f1), std::ref(f2),
                         benchmark_name, i + 1, iterations);
  }

  // Wait for threads to finish
  for (int i = 0; i < thread_count; ++i)
  {
    threads[i].join();
  }

  auto const end = std::chrono::steady_clock::now();

  uint64_t const time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << "********************************* " << std::endl;
  std::cout << benchmark_name << " - Thread count: " << thread_count
            << " Total Messages: " << thread_count * iterations
            << " Total Time Elapsed: " << time_elapsed << " Millisecond\n"
            << std::endl;
}

int main(int argc, char* argv[])
{

  if (argc != 2)
  {
    std::cerr << "Please provide the name of the logger as argument." << std::endl;
    return 0;
  }

  std::array<int32_t, 4> threads_num{{1, 2, 3, 4}};
  uint32_t const iterations = 100'000;

  // Main thread is not important so set it to the same cpu as the backend
  quill::detail::set_cpu_affinity(0);

  if (strcmp(argv[1], "quill") == 0)
  {
    // Setup

    // Pin the backend thread to cpu 0
    quill::config::set_backend_thread_cpu_affinity(0);
    quill::config::set_backend_thread_sleep_duration(std::chrono::nanoseconds{0});

    // Start the logging backend thread
    quill::start();

    // wait for the backend thread to start
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Create a file handler to write to a file
    quill::Handler* file_handler =
      quill::file_handler("quill_total_time_until_completion_benchmark", "w");

    quill::Logger* logger = quill::create_logger("bench_logger", file_handler);

    // Define a logging lambda
    auto quill_benchmark = [logger](int32_t i, double d, char const* str) {
      LOG_INFO(logger, "Logging str: {}, int: {}, double: {}", str, i, d);
    };

    auto on_exit = []() {
      // Flush all logs before terminating
      quill::flush();
    };

    // Run the benchmark for n threads
    for (auto threads : threads_num)
    {
      run_benchmark(quill_benchmark, on_exit, threads,
                    "Logger: Quill - Benchmark: Total Time Until Completion", iterations);
    }
  }

  std::remove("quill_total_time_until_completion_benchmark");
  return 0;
}