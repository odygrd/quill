#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("multi_frontend_threads")
{
  static constexpr size_t number_of_messages = 1000;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* filename = "multi_frontend_threads.log";
  static std::string const logger_name_prefix = "logger_";

  // Start the logging backend thread
  Backend::start();

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

        Logger* logger =
          Frontend::create_or_get_logger(logger_name_prefix + std::to_string(i), std::move(file_sink));

        for (size_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger, "Hello from thread {} this is message {}", i, j);
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

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
  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads);

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

  testing::remove_file(filename);
}