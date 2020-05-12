#define QUILL_QUEUE_CAPACITY 262'144

#include "quill/Quill.h"
#include "quill/detail/misc/RdtscClock.h"
#include <chrono>

static constexpr size_t total_iterations = 2'000'000;

/**
 * The backend worker just spins, so we just measure the total time elapsed for total_iterations
 */
int main()
{
  quill::detail::RdtscClock rdtsc_clock;

  // main thread affinity
  quill::detail::set_cpu_affinity(0);

  /** - Setup Quill **/
  quill::config::set_backend_thread_sleep_duration(std::chrono::nanoseconds{0});

  // set backend on it's own cpu
  quill::config::set_backend_thread_cpu_affinity(1);

  // Start the logging backend thread
  quill::start();

  // Create a file handler to write to a file
  quill::Handler* file_handler = quill::file_handler("quill_backend_total_time.log", "w");

  quill::Logger* logger = quill::create_logger("bench_logger", file_handler);

  quill::preallocate();

  // start counting the time until backend worker finishes
  unsigned int aux;
  auto const start = __rdtscp(&aux);

  for (size_t iteration = 0; iteration < total_iterations; ++iteration)
  {
    LOG_INFO(logger, "Iteration: {}, int: {}, double: {}", iteration, iteration * 2,
             static_cast<double>(iteration) / 2);
  }

  // block until all messages are flushed
  quill::flush();
  auto const end = __rdtscp(&aux);

  uint64_t const latency{static_cast<uint64_t>((end - start) / rdtsc_clock.ticks_per_nanosecond())};
  std::chrono::nanoseconds latency_ns = std::chrono::nanoseconds{latency};

  std::cout << "Iterations: " << total_iterations << "\nTotal time elapsed: " << latency_ns.count()
            << " ns, " << std::chrono::duration_cast<std::chrono::microseconds>(latency_ns).count()
            << " us, " << std::chrono::duration_cast<std::chrono::milliseconds>(latency_ns).count()
            << " ms" << std::endl;
}