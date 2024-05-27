#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backend_transit_buffer_soft_limit")
{
  static constexpr char const* filename = "backend_transit_buffer_soft_limit.log";
  static std::string const logger_name = "logger";
  size_t constexpr soft_limit = 100;

  // First log some messages and then start the backend worker thread so that the soft limit is hit

  // Set writing logging to a file
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

  for (size_t i = 0; i < soft_limit * 2; ++i)
  {
    // log all messages
    LOG_INFO(logger, "Message num {}", i);
  }

  // Start the backend thread
  BackendOptions backend_options;
  backend_options.transit_events_soft_limit = soft_limit;
  Backend::start(backend_options);

  // log more messages after the backend started
  for (size_t i = soft_limit * 2; i < soft_limit * 4; ++i)
  {
    // log all messages
    LOG_INFO(logger, "Message num {}", i);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);

  size_t const total_messages = soft_limit * 4;
  REQUIRE_EQ(file_contents.size(), total_messages);

  for (size_t i = 0; i < total_messages; ++i)
  {
    REQUIRE(quill::testing::file_contains(
      file_contents, std::string{"LOG_INFO      " + logger_name + "       Message num "} + std::to_string(i)));
  }

  testing::remove_file(filename);
}
