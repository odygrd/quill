#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Quill.h"
#include "quill/detail/misc/FileUtilities.h"
#include "quill/handlers/JsonFileHandler.h"
#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("QuillStructuredLog");

// Note: This thread is flushing using the main() gtest test thread. This means that no-other test should have used flush()
// on the main gtest test thread as the main thread's thread context is not re-added.
// Other tests use flush() but within their spawned threads.
// this is never an issue in real logger as everything goes through the singleton, but we are not using the
// singleton all the time during testing

void test_quill_log(char const* test_id, std::string const& filename, std::string const& filename_s,
                    uint16_t number_of_threads, uint32_t number_of_messages)
{
  // Start the logging backend thread
  quill::start();

  std::vector<std::thread> threads;

  // log to json
  std::shared_ptr<quill::Handler> log_from_one_thread_file =
    quill::json_file_handler(filename,
                             []()
                             {
                               quill::JsonFileHandlerConfig cfg;
                               cfg.set_open_mode('w');
                               return cfg;
                             }());

  // It is not thread safe to call set_pattern
  log_from_one_thread_file->set_pattern("", std::string{"%Y-%m-%d %H:%M:%S.%Qus"});

  for (int i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [log_from_one_thread_file, filename, filename_s, number_of_messages, test_id, i]() mutable
      {
        // Also use preallocate
        quill::preallocate();

        // log non structured file
        std::shared_ptr<quill::Handler> log_s_from_one_thread_file =
          quill::file_handler(filename_s,
                              []()
                              {
                                quill::FileHandlerConfig cfg;
                                cfg.set_open_mode('w');
                                return cfg;
                              }());

        std::string logger_name = "jlogger_" + std::string{test_id} + "_" + std::to_string(i);
        quill::Logger* logger = quill::create_logger(
          logger_name,
          std::vector<std::shared_ptr<quill::Handler>>{std::move(log_from_one_thread_file),
                                                       std::move(log_s_from_one_thread_file)});

        for (uint32_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger, "Hello from thread {thread_index} this is message {message_num}", i, j);
        }
      });
  }

  // we can release the pointer now so the Handler is destroyed and the file is closed
  log_from_one_thread_file.reset();

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
  std::vector<std::string> const file_contents_s = quill::testing::file_contents(filename_s);

  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads);
  REQUIRE_EQ(file_contents_s.size(), number_of_messages * number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    for (uint32_t j = 0; j < number_of_messages; ++j)
    {
      // check json log
      std::string expected_logger_name = "jlogger_" + std::string{test_id} + "_" + std::to_string(i);

      std::string expected_json_string = std::string{"\"logger\": \""} + expected_logger_name +
        std::string{
          "\", \"level\": \"INFO\", \"message\": \"Hello from thread {thread_index} this is "
          "message {message_num}\", "} +
        std::string{"\"thread_index\": \""} + std::to_string(i) +
        std::string{"\", \"message_num\": \""} + std::to_string(j) + std::string{"\""};

      REQUIRE(quill::testing::file_contains(file_contents, expected_json_string));

      // check standard log
      // for each thread
      std::string expected_string = expected_logger_name + " Hello from thread " +
        std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(quill::testing::file_contains(file_contents_s, expected_string));
    }
  }

  quill::detail::remove_file(filename);
  quill::detail::remove_file(filename_s);
}

/***/
TEST_CASE("log_json_from_multiple_threads")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 6;
  static constexpr char const* test_id = "multi";

  static constexpr char const* filename = "log_json_from_multiple_threads.log";
  static constexpr char const* filename_s = "log_s_from_multiple_threads.log";
  test_quill_log(test_id, filename, filename_s, number_of_threads, number_of_messages);
}

TEST_SUITE_END();