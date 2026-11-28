/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

/**
 * The hot path latency of the macro-free functions (quill::info() etc.) with the default unbounded
 * queue. Built once per payload and scenario, see hot_path_bench_config.h and CMakeLists.txt.
 */

#include "hot_path_bench.h"
#include "quill_hot_path_frontend_options.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"
#include "quill/std/Vector.h"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using frontend_t = quill::FrontendImpl<CustomFrontendOptions>;
using logger_t = quill::LoggerImpl<CustomFrontendOptions>;

/***/
void quill_benchmark(std::vector<int32_t> const& thread_count_array, size_t num_iterations_per_thread)
{
  std::remove(BENCH_LOG_FILENAME);

  // Setup
  quill::BackendOptions backend_options;
#if !defined(__APPLE__) || !defined(__aarch64__)
  backend_options.cpu_affinity = {static_cast<uint16_t>(bench_cpu(5))};
#endif
  // The backend never sleeps, the most responsive setting
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  quill::Backend::start(backend_options);

  // wait for the backend thread to start
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // Frontend
  auto file_sink = frontend_t::create_or_get_sink<quill::FileSink>(
    BENCH_LOG_FILENAME,
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_filename_append_option(quill::FilenameAppendOption::None);
      return cfg;
    }(),
    quill::FileEventNotifier{});

  logger_t* logger = frontend_t::create_or_get_logger(
    "root", std::move(file_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "LOG_%(log_level:<9) %(logger:<12) %(message)",
                                   "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false});

  // Measure enabled logging regardless of the application environment.
  logger->set_log_level(quill::LogLevel::Info);

  // Define a logging lambda
#ifdef BENCH_SCENARIO_DISABLED
  // A statement below the logger's level
  char const* benchmark_type = "[benchmark_type: int_int_double]";

  auto log_func = [logger](uint64_t i, uint64_t j, double d)
  { quill::debug(logger, "Logging int: {}, int: {}, double: {}", i, j, BENCH_DISABLED_ARG(d)); };
#elif defined(BENCH_INT_INT_DOUBLE)
  char const* benchmark_type = "[benchmark_type: int_int_double]";

  auto log_func = [logger](uint64_t i, uint64_t j, double d)
  { quill::info(logger, "Logging int: {}, int: {}, double: {}", i, j, d); };
#elif defined(BENCH_INT_INT_LARGESTR)
  char const* benchmark_type = "[benchmark_type: int_int_largestr]";

  auto log_func = [logger](uint64_t i, uint64_t j, std::string const& s)
  { quill::info(logger, "Logging int: {}, int: {}, string: {}", i, j, s); };
#elif defined(BENCH_VECTOR_LARGESTR)
  char const* benchmark_type = "[benchmark_type: vector_largestr]";

  auto log_func = [logger](uint64_t i, uint64_t j, std::vector<std::string> const& s)
  { quill::info(logger, "Logging int: {}, int: {}, vector: {}", i, j, s); };
#else
  static_assert(false,
                "define BENCH_INT_INT_DOUBLE, BENCH_INT_INT_LARGESTR, or BENCH_VECTOR_LARGESTR");
#endif

  auto on_start = [logger]()
  {
    quill::info(logger, "Warm up");
    logger->flush_log(0);
  };

  // Block until the backend has written everything, so the next run starts with an idle backend
  auto on_exit = [logger]() { logger->flush_log(); };

  // Run the benchmark for n threads
  std::string benchmark_name = "Logger: Quill - Benchmark: Caller Thread Latency, Macro Free Mode";
  benchmark_name += " - ";
  benchmark_name += benchmark_type;

  for (auto thread_count : thread_count_array)
  {
    run_benchmark(benchmark_name.c_str(), thread_count, num_iterations_per_thread,
                  MESSAGES_PER_ITERATION, on_start, log_func, on_exit);
  }
}

/***/
int main() { quill_benchmark(THREAD_LIST_COUNT, ITERATIONS); }
