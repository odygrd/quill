#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Quill.h"
#include "quill/detail/misc/FileUtilities.h"
#include "quill/handlers/JsonFileHandler.h"
#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("QuillLogNoTransit");

// Note: This thread is flushing using the main() gtest test thread. This means that no-other test should have used flush()
// on the main gtest test thread as the main thread's thread context is not re-added.
// Other tests use flush() but within their spawned threads.
// this is never an issue in real logger as everything goes through the singleton, but we are not using the
// singleton all the time during testing

void test_quill_log(char const* test_id, std::string const& filename, uint16_t number_of_threads,
                    uint32_t number_of_messages)
{
  quill::Config cfg;
  cfg.backend_thread_use_transit_buffer = false;
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  std::vector<std::thread> threads;

  for (int i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [filename, number_of_messages, test_id, i]() mutable
      {
        // Also use preallocate
        quill::preallocate();

        // Set writing logging to a file
        std::shared_ptr<quill::Handler> log_from_one_thread_file =
          quill::file_handler(filename,
                              []()
                              {
                                quill::FileHandlerConfig cfg;
                                cfg.set_open_mode('w');
                                return cfg;
                              }());

        std::string logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);
        quill::Logger* logger = quill::create_logger(logger_name.data(), std::move(log_from_one_thread_file));

        for (uint32_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger, "Hello from thread {thread_index} this is message {message_num}", i, j);
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // Flush all log
  quill::flush();

  for (auto [key, value] : quill::get_all_loggers())
  {
    quill::remove_logger(value);
  }

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    for (uint32_t j = 0; j < number_of_messages; ++j)
    {
      std::string expected_logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);

      std::string expected_string = expected_logger_name + " Hello from thread " +
        std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  quill::detail::remove_file(filename);
}

/***/
TEST_CASE("log_from_multiple_threads_no_transit_buffer")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* test_id = "multi";

  static constexpr char const* filename = "log_from_multiple_threads_no_transit_buffer.log";
  test_quill_log(test_id, filename, number_of_threads, number_of_messages);
}
