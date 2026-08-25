#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

static constexpr size_t iterations_per_thread = 2'000'000;
static constexpr size_t total_iterations = iterations_per_thread * 2;

/**
 * Measures total backend throughput and timestamp ordering with two concurrent producers.
 */
int main()
{
  quill::BackendOptions backend_options;
  backend_options.cpu_affinity = {5};
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{1};

  quill::Backend::start(backend_options);

  std::shared_ptr<quill::Sink> file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    "quill_backend_throughput_two_producers.log",
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

  LOG_INFO(logger, "preallocate");
  logger->flush_log(0);

  std::atomic<uint32_t> ready_threads{0};
  std::atomic<bool> start{false};

  auto producer = [logger, &ready_threads, &start](uint16_t cpu)
  {
#if defined(QUILL_NO_EXCEPTIONS)
    quill::detail::set_cpu_affinity({cpu});
#else
    try
    {
      quill::detail::set_cpu_affinity({cpu});
    }
    catch (std::exception const& e)
    {
      std::cerr << "Failed to set cpu affinity: " << e.what() << std::endl;
    }
#endif

    ready_threads.fetch_add(1, std::memory_order_release);
    while (!start.load(std::memory_order_acquire))
    {
    }

    for (size_t iteration = 0; iteration < iterations_per_thread; ++iteration)
    {
      LOG_INFO(logger, "Iteration: {} int: {} double: {}", iteration, iteration * 2,
               static_cast<double>(iteration) / 2);
    }
  };

  std::thread first_producer{producer, static_cast<uint16_t>(1)};
  std::thread second_producer{producer, static_cast<uint16_t>(2)};

  while (ready_threads.load(std::memory_order_acquire) != 2)
  {
  }

  auto const start_time = std::chrono::steady_clock::now();
  start.store(true, std::memory_order_release);

  first_producer.join();
  second_producer.join();
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
