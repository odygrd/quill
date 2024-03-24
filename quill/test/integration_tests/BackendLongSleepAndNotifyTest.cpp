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
TEST_CASE("backend_long_sleep_and_notify")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* filename = "log_backend_long_sleep_and_notify.log";
  static std::string const logger_name_prefix = "logger_";

  // Start the backend thread
  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::hours{24};
  Backend::start(backend_options);

  // wait for backend worker to start
  std::this_thread::sleep_for(std::chrono::seconds{1});

  std::vector<std::thread> threads;

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [i]() mutable
      {
        // Also use preallocate
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
          LOG_INFO(logger, "Hello from thread {thread_index} this is message {message_num}", i, j);
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // The backend worker is still sleeping here so no file should exist
  REQUIRE_EQ(testing::file_contents(filename).size(), 0);

  // Flush all log and remove the loggers
  REQUIRE_FALSE(Frontend::get_all_loggers().empty());

  for (Logger* logger : Frontend::get_all_loggers())
  {
    // wake up the backend logging thread on demand
    // This is needed here to wake up the backend thread to even process the flush_log_message
    Backend::notify();

    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(filename);

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
