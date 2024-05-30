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
TEST_CASE("log_flush_with_soft_limit_test")
{
  static constexpr char const* filename = "log_flush_with_soft_limit_test.log";
  static std::string const logger_name = "logger";
  static size_t constexpr soft_limit = 100;

  // make sure we at least x 100 the soft limit to make sure we are testing what we want
  // to not reduce this number. Increasing is fine
  static size_t constexpr number_of_messages = soft_limit * 100;
  static constexpr size_t number_of_threads = 4;

  // When hitting the transit_events_soft_limit several events are processed in the backend at the
  // same time If all of them are processed there is a chance to miss messages that where in the
  // queues but never buffered result in example issuing the flush_log() earlier than it should

  // Start the backend thread
  BackendOptions backend_options;
  backend_options.transit_events_soft_limit = soft_limit;
  Backend::start(backend_options);

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
            cfg.set_do_fsync(true);
            return cfg;
          }(),
          FileEventNotifier{});

        Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

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

  Frontend::get_valid_logger()->flush_log();

  // Now check we have all messages in the file after the flush statement

  size_t const total_messages = number_of_messages * number_of_threads;
  // Read file and check
  REQUIRE_EQ(testing::file_contents(filename).size(), total_messages);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check again, nothing else was logged
  REQUIRE_EQ(testing::file_contents(filename).size(), total_messages);

  testing::remove_file(filename);
}
