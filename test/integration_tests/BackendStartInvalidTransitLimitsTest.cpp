#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backend_start_invalid_transit_limits")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  static constexpr size_t number_of_messages = 50;
  static constexpr char const* filename = "backend_start_invalid_transit_limits.log";
  static std::string const logger_name = "logger_invalid_transit_limits";

  // Invalid transit event limits must throw from Backend::start() on the caller thread instead
  // of terminating the process from the backend thread
  {
    BackendOptions backend_options;
    backend_options.transit_events_hard_limit = 100'000; // not a power of two
    REQUIRE_THROWS_AS(Backend::start(backend_options), QuillError);
    REQUIRE_FALSE(Backend::is_running());
  }

  {
    BackendOptions backend_options;
    backend_options.transit_event_buffer_initial_capacity = 257; // rounds up to 512
    backend_options.transit_events_hard_limit = 256;
    backend_options.transit_events_soft_limit = 256;
    REQUIRE_THROWS_AS(Backend::start(backend_options), QuillError);
    REQUIRE_FALSE(Backend::is_running());
  }

  {
    BackendOptions backend_options;
    backend_options.transit_events_soft_limit = 8192;
    backend_options.transit_events_hard_limit = 4096; // soft > hard
    REQUIRE_THROWS_AS(Backend::start(backend_options), QuillError);
    REQUIRE_FALSE(Backend::is_running());
  }

  if constexpr (sizeof(size_t) == sizeof(uint32_t))
  {
    // On 32-bit targets next_power_of_two() saturates at the top bit. The unrounded request must
    // also be checked so UINT32_MAX cannot masquerade as a valid 2^31 initial capacity.
    BackendOptions backend_options;
    backend_options.transit_event_buffer_initial_capacity = (std::numeric_limits<uint32_t>::max)();
    backend_options.transit_events_soft_limit = 1;
    backend_options.transit_events_hard_limit = ((std::numeric_limits<size_t>::max)() >> 1u) + 1u;
    REQUIRE_THROWS_AS(Backend::start(backend_options), QuillError);
    REQUIRE_FALSE(Backend::is_running());
  }

  // After a failed start, a valid configuration must still be able to start and log
  BackendOptions backend_options;
  backend_options.transit_events_soft_limit = 800; // arbitrary thresholds are supported
  Backend::start(backend_options);
  REQUIRE(Backend::is_running());

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

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "This is message {}", i);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string expected_string = logger_name + " This is message " + std::to_string(i);
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  testing::remove_file(filename);
#endif
}
