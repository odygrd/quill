/**
 * Adding a benchmark for a another logger should be straight forward by duplicating and modifying
 * this file.
 */

#include "hot_path_bench.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

struct FrontendOptions
{
  static constexpr quill::QueueType queue_type = quill::QueueType::UnboundedBlocking;
  static constexpr uint32_t initial_queue_capacity = 131'072;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr bool huge_pages_enabled = false;
};

using Frontend = quill::FrontendImpl<FrontendOptions>;
using Logger = quill::LoggerImpl<FrontendOptions>;

/***/
void quill_benchmark(std::vector<uint16_t> const& thread_count_array,
                     size_t num_iterations_per_thread, size_t messages_per_iteration)
{
  /** - MAIN THREAD START - Logger setup if any **/

  /** - Setup Quill **/
  // main thread affinity
  quill::detail::set_cpu_affinity(0);

  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = 5;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};

  // Start the logging backend thread and give it some tiem to init
  quill::Backend::start(backend_options);

  std::this_thread::sleep_for(std::chrono::milliseconds{100});

  // wait for the backend thread to start
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Create a file sink to write to a file
  std::shared_ptr<quill::Sink> file_sink = Frontend::create_or_get_sink<quill::FileSink>(
    "quill_hot_path_rdtsc_clock.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "bench_logger", std::move(file_sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location) %(log_level) %(message)", "%H:%M:%S.%Qns",
      quill::Timezone::LocalTime, false},
    quill::ClockSourceType::System);

  /** LOGGING THREAD FUNCTIONS - on_start, on_exit, log_func must be implemented **/
  /** those run on a several thread(s). It can be one or multiple threads based on THREAD_LIST_COUNT config */
  auto on_start = []()
  {
    // on thread start
    Frontend::preallocate();
  };

  auto on_exit = [logger]()
  {
    // on thread exit we block flush, so the next benchmark starts with the backend thread ready
    // to process the messages
    logger->flush_log();
  };

  // on main
  auto log_func = [logger](uint64_t k, uint64_t i, double d)
  {
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