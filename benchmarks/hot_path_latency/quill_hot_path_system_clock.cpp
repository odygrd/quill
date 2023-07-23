/**
 * Adding a benchmark for a another logger should be straight forward by duplicating and modifying
 * this file.
 */

#include "hot_path_bench.h"
#include "quill/Quill.h"

/***/
void quill_benchmark(std::vector<int32_t> const& thread_count_array,
                     size_t num_iterations_per_thread, size_t messages_per_iteration)
{
  std::remove("quill_call_site_latency_percentile_linux_benchmark.log");

  /** - MAIN THREAD START - Logger setup if any **/

  /** - Setup Quill **/
  quill::Config cfg;
  cfg.backend_thread_yield = false;
  cfg.backend_thread_cpu_affinity = 0;
  cfg.default_timestamp_clock_type = quill::TimestampClockType::System;

  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  // wait for the backend thread to start
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Create a file handler to write to a file
  std::shared_ptr<quill::Handler> file_handler =
    quill::file_handler("quill_hot_path_system_clock.log",
                        []()
                        {
                          quill::FileHandlerConfig cfg;
                          cfg.set_open_mode('w');
                          return cfg;
                        }());

  quill::Logger* logger = quill::create_logger("bench_logger", std::move(file_handler));

  /** LOGGING THREAD FUNCTIONS - on_start, on_exit, log_func must be implemented **/
  /** those run on a several thread(s). It can be one or multiple threads based on THREAD_LIST_COUNT config */
  auto on_start = []() {
    // on thread start
    quill::preallocate();
  };

  auto on_exit = []() {
    // on thread exit we block flush, so the next benchmark starts with the backend thread ready
    // to process the messages
    quill::flush();
  };

  // on main
  auto log_func = [logger](uint64_t k, uint64_t i, double d) {
    // Main logging function
    // This will get called MESSAGES_PER_ITERATION * ITERATIONS for each caller thread.
    // MESSAGES_PER_ITERATION will get averaged to a single number

    LOG_INFO(logger, "Logging iteration: {}, message: {}, double: {}", k, i, d);
  };

  /** ALWAYS REQUIRED **/
  // Run the benchmark for n threads
  for (auto thread_count : thread_count_array)
  {
    run_benchmark("Logger: Quill - Benchmark: Hot Path Latency / Nanoseconds", thread_count,
                  num_iterations_per_thread, messages_per_iteration, on_start, log_func, on_exit);
  }
}

/***/
int main(int, char**) { quill_benchmark(THREAD_LIST_COUNT, ITERATIONS, MESSAGES_PER_ITERATION); }