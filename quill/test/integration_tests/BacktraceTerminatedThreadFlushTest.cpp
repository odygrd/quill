#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("backtrace_terminated_thread_flush")
{
  // In this test we store in backtrace from one thread that terminates
  // then we log that backtrace from a different thread

  static constexpr char const* filename = "backtrace_terminated_thread_flush.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  std::thread thread_a(
    []()
    {
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

      // Get a logger and enable backtrace
      Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

      // Enable backtrace for 2 messages
      logger->init_backtrace(2, LogLevel::Error);

      LOG_INFO(logger, "Before backtrace log");
      for (uint32_t i = 0; i < 12; ++i)
      {
        LOG_BACKTRACE(logger, "Backtrace message {}", i);
      }
    });

  thread_a.join();

  // thread_a logged something in backtrace and finished.
  // Now we spawn a different thread and LOG_ERROR
  // we expect to see the backtrace from the previous thread
  std::thread thread_b(
    []()
    {
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

      // Get a logger and enable backtrace
      Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

      LOG_ERROR(logger, "After error");

      logger->flush_log();
      Frontend::remove_logger(logger);
    });

  thread_b.join();

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), 4);

  std::string expected_string_1 = "LOG_INFO      " + logger_name + "       Before backtrace log";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_1));

  std::string expected_string_2 = "LOG_ERROR     " + logger_name + "       After error";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_2));

  std::string expected_string_3 = "LOG_BACKTRACE " + logger_name + "       Backtrace message 10";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_3));

  std::string expected_string_4 = "LOG_BACKTRACE " + logger_name + "       Backtrace message 11";
  REQUIRE(quill::testing::file_contains(file_contents, expected_string_4));

  testing::remove_file(filename);
}