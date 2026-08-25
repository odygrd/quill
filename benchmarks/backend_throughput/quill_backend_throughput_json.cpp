#include <chrono>
#include <iostream>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/JsonSink.h"

static constexpr size_t total_iterations = 4'000'000;

/**
 * Measures total backend throughput when serializing positional arguments to JSON.
 */
int main()
{
#if defined(QUILL_NO_EXCEPTIONS)
  quill::detail::set_cpu_affinity({1});
#else
  try
  {
    quill::detail::set_cpu_affinity({1});
  }
  catch (std::exception const& e)
  {
    std::cerr << "Failed to set cpu affinity: " << e.what() << std::endl;
  }
#endif

  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = {5};
  backend_options.sleep_duration = std::chrono::nanoseconds{0};

  quill::Backend::start(backend_options);

  std::shared_ptr<quill::Sink> file_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(
    "quill_backend_throughput_json.log",
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "bench_logger", std::move(file_sink),
    quill::PatternFormatterOptions{"", "%H:%M:%S.%Qns", quill::Timezone::LocalTime, false});

  LOG_INFO(logger, "preallocate");
  logger->flush_log(0);

  auto const start_time = std::chrono::steady_clock::now();
  for (size_t iteration = 0; iteration < total_iterations; ++iteration)
  {
    LOG_INFO(logger, "Iteration: {} int: {} double: {}", iteration, iteration * 2,
             static_cast<double>(iteration) / 2);
  }

  logger->flush_log(0);

  auto const end_time = std::chrono::steady_clock::now();
  auto const delta = end_time - start_time;
  auto const delta_d = std::chrono::duration_cast<std::chrono::duration<double>>(delta).count();

  std::cout << fmtquill::format(
                 "Throughput is {:.2f} million msgs/sec average, total time elapsed: {} ms for {} "
                 "log messages \n",
                 total_iterations / delta_d / 1e6,
                 std::chrono::duration_cast<std::chrono::milliseconds>(delta).count(), total_iterations)
            << std::endl;
}
