#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BackendTscClock.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backend_tsc_clock")
{
  static constexpr char const* filename = "backend_tsc_clock.log";
  static std::string const logger_name = "logger";

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // Backend is not running yet, fallback to system_clock
  REQUIRE_GE(BackendTscClock::now().time_since_epoch().count(), 0);

  // Backend is not running yet
  BackendTscClock::RdtscVal tsc = BackendTscClock::rdtsc();
  REQUIRE_EQ(BackendTscClock::to_time_point(tsc).time_since_epoch().count(), 0);

  Backend::start();

  // For the backend to initialise the TscClock we need to log at least once
  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  LOG_INFO(logger, "Init");
  logger->flush_log();

  // Backend is now running
  auto const system_now = std::chrono::system_clock::now().time_since_epoch();
  auto const backend_tsc_time_1 = BackendTscClock::now().time_since_epoch();

  static auto const ten_minutes = std::chrono::minutes(10);

  // Check that tsc_now is within 10 minutes of system_now
  REQUIRE_GE(backend_tsc_time_1, system_now - ten_minutes);
  REQUIRE_LE(backend_tsc_time_1, system_now + ten_minutes);

  // Check same for calling it with an explicit value
  BackendTscClock::RdtscVal const backend_tsc = BackendTscClock::rdtsc();
  auto const backend_tsc_time_2 = BackendTscClock::to_time_point(backend_tsc).time_since_epoch();

  REQUIRE_GE(backend_tsc_time_2, system_now - ten_minutes);
  REQUIRE_LE(backend_tsc_time_2, system_now + ten_minutes);

  // Repeated stop/start must not invalidate readers that loaded the clock before stop published
  // nullptr. Restarts alternate resync intervals to exercise reconfiguration of the same stable
  // clock allocation.
  static constexpr size_t restart_cycles = 32;
  static constexpr size_t reader_count = 4;

  for (size_t cycle = 0; cycle < restart_cycles; ++cycle)
  {
    std::atomic<bool> keep_reading{true};
    std::atomic<size_t> readers_started{0};
    std::vector<std::thread> readers;
    readers.reserve(reader_count);

    for (size_t i = 0; i < reader_count; ++i)
    {
      readers.emplace_back(
        [&keep_reading, &readers_started]()
        {
          readers_started.fetch_add(1, std::memory_order_release);
          while (keep_reading.load(std::memory_order_acquire))
          {
            (void)BackendTscClock::now();
          }
        });
    }

    while (readers_started.load(std::memory_order_acquire) != reader_count)
    {
      std::this_thread::yield();
    }

    Backend::stop();

    BackendTscClock::RdtscVal const stopped_tsc = BackendTscClock::rdtsc();
    REQUIRE_EQ(BackendTscClock::to_time_point(stopped_tsc).time_since_epoch().count(), 0);
    REQUIRE_GE(BackendTscClock::now().time_since_epoch().count(), 0);

    if ((cycle + 1) != restart_cycles)
    {
      BackendOptions restart_options;
      restart_options.rdtsc_resync_interval =
        ((cycle % 2) == 0) ? std::chrono::milliseconds{250} : std::chrono::milliseconds{750};
      Backend::start(restart_options);
      LOG_INFO(logger, "Init after restart {}", cycle);
      logger->flush_log();

      auto const restarted_system_now = std::chrono::system_clock::now().time_since_epoch();
      auto const restarted_tsc_now = BackendTscClock::now().time_since_epoch();
      REQUIRE_GE(restarted_tsc_now, restarted_system_now - ten_minutes);
      REQUIRE_LE(restarted_tsc_now, restarted_system_now + ten_minutes);
    }

    keep_reading.store(false, std::memory_order_release);

    for (std::thread& reader : readers)
    {
      reader.join();
    }
  }

  testing::remove_file(filename);
}
