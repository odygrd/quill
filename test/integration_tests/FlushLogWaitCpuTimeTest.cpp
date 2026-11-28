#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif
  #if !defined(NOMINMAX)
    #define NOMINMAX
  #endif
  #include <windows.h>
#endif

using namespace quill;

#if defined(_WIN32)
/***/
uint64_t current_thread_cpu_time_ns()
{
  FILETIME creation_time;
  FILETIME exit_time;
  FILETIME kernel_time;
  FILETIME user_time;
  REQUIRE_NE(::GetThreadTimes(::GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time), 0);

  auto const to_ns = [](FILETIME const& time)
  {
    // FILETIME counts 100 ns intervals
    return ((static_cast<uint64_t>(time.dwHighDateTime) << 32u) | time.dwLowDateTime) * 100u;
  };

  return to_ns(kernel_time) + to_ns(user_time);
}
#endif

/***/
TEST_CASE("flush_log_wait_cpu_time")
{
#if !defined(_WIN32)
  // The thread's CPU time is read with GetThreadTimes(). On Windows sleep_for_ns() yields for a
  // wait below 1 ms, which flush_log() must not use to wait for the backend
  return;
#else
  static constexpr size_t number_of_flushes = 5;
  static constexpr char const* filename = "flush_log_wait_cpu_time.log";
  static std::string const logger_name = "flush_log_wait_cpu_time_logger";

  // The backend is never notified, so each flush_log() waits for the rest of its 200 ms sleep
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::milliseconds{200};
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  auto const wall_start = std::chrono::steady_clock::now();
  uint64_t const cpu_start_ns = current_thread_cpu_time_ns();

  for (size_t i = 0; i < number_of_flushes; ++i)
  {
    LOG_INFO(logger, "Flush number {}", i);
    logger->flush_log();
  }

  uint64_t const cpu_time_ns = current_thread_cpu_time_ns() - cpu_start_ns;
  auto const wall_time_ns = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - wall_start)
      .count());

  // A thread that yields while it waits uses about as much CPU time as the waits last. One that
  // sleeps a timer tick between its checks uses a few milliseconds
  REQUIRE_LT(cpu_time_ns, wall_time_ns / 2);

  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_flushes);

  testing::remove_file(filename);
#endif
}
