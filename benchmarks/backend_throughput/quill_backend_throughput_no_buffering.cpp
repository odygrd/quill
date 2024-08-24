#include <chrono>
#include <iostream>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

static constexpr size_t total_iterations = 4'000'000;

/**
 * The backend worker just spins, so we just measure the total time elapsed for total_iterations
 */
int main()
{
  // main thread affinity
  quill::detail::set_cpu_affinity(0);

  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = 5;
  backend_options.transit_events_hard_limit = 1;

  // Start the logging backend thread and give it some tiem to init
  quill::Backend::start(backend_options);

  std::this_thread::sleep_for(std::chrono::milliseconds{100});

  // Create a file sink to write to a file
  std::shared_ptr<quill::Sink> file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "quill_backend_total_time.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "bench_logger", std::move(file_sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location) %(log_level) %(message)", "%H:%M:%S.%Qns",
      quill::Timezone::LocalTime, false});
  ;

  quill::Frontend::preallocate();

  // start counting the time until backend worker finishes
  auto const start_time = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < total_iterations; ++iteration)
  {
    LOG_INFO(logger, "Iteration: {} int: {} double: {}", iteration, iteration * 2,
             static_cast<double>(iteration) / 2);
  }

  // block until all messages are flushed
  logger->flush_log(0);

  auto const end_time = std::chrono::steady_clock::now();
  auto const delta = end_time - start_time;
  auto delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();

  std::cout << fmtquill::format(
                 "Throughput is {:.2f} million msgs/sec average, total time elapsed: {} ms for {} "
                 "log messages \n",
                 total_iterations / delta_d / 1e6,
                 std::chrono::duration_cast<std::chrono::milliseconds>(delta).count(), total_iterations)
            << std::endl;
}