#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/BackendTscClock.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

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

  testing::remove_file(filename);
}
