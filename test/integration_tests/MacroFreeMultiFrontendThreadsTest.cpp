#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/sinks/FileSink.h"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("macro_free_multi_frontend_threads")
{
  static constexpr size_t number_of_messages = 1000;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* filename = "macro_free_multi_frontend_threads.log";
  static std::string const logger_name_prefix = "logger_";

  BackendOptions bo;
  bo.transit_event_buffer_initial_capacity = 2;
  Backend::start(bo);

  std::vector<std::thread> threads;

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [i]() mutable
      {
        Frontend::preallocate();

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

        Logger* logger = Frontend::create_or_get_logger(
          logger_name_prefix + std::to_string(i), std::move(file_sink),
          quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
            "%(message)",
            "%Y-%m-%d %H:%M:%S.%Qns", Timezone::GmtTime, false},
          ClockSourceType::System);

        for (size_t j = 0; j < number_of_messages; ++j)
        {
          info(logger, "Hello from thread {} this is message {}", i, j);
        }
      });
  }

  // wait all threads to join
  for (auto& elem : threads)
  {
    elem.join();
  }

  // log from main thread - get logger from the first thread
  Logger* t0_logger = Frontend::get_logger(logger_name_prefix + std::to_string(0));
  REQUIRE(t0_logger);

  t0_logger->set_log_level(quill::LogLevel::TraceL3);

  tracel3(t0_logger, "a trace_l3 message");
  tracel2(t0_logger, "a trace_l2 message");
  tracel1(t0_logger, "a trace_l1 message");
  debug(t0_logger, "a debug message");
  info(t0_logger, "an info message");
  notice(t0_logger, "a notice message");
  warning(t0_logger, "a warning message");
  error(t0_logger, "an error message");
  critical(t0_logger, "a critical message");

  // with nullptr
  Logger* null_logger = nullptr;
  info(null_logger, "null");
  warning(null_logger, "null");

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads + 9);

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    for (size_t j = 0; j < number_of_messages; ++j)
    {
      std::string expected_string = logger_name_prefix + std::to_string(i) +
        "     Hello from thread " + std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(testing::file_contains(file_contents, expected_string));
    }
  }

  REQUIRE(testing::file_contains(file_contents, "LOG_TRACE_L3  logger_0     a trace_l3 message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_TRACE_L2  logger_0     a trace_l2 message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_TRACE_L1  logger_0     a trace_l1 message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_DEBUG     logger_0     a debug message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_INFO      logger_0     an info message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_NOTICE    logger_0     a notice message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_WARNING   logger_0     a warning message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_ERROR     logger_0     an error message"));
  REQUIRE(testing::file_contains(file_contents, "LOG_CRITICAL  logger_0     a critical message"));

  testing::remove_file(filename);
}