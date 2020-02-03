#include <gtest/gtest.h>

#include "misc/TestUtilities.h"
#include "quill/Quill.h"
#include <string>
#include <cstdio>

// Note: This thread is flushing using the main thread. This means that no-other test should have used flush()
// on the main thread as the main thread's thread context is not re-added
// this is never an issue in real logger as everything goes through the singleton, but we are not using the
// singleton all the time during testing

void test_quil_log(char const* test_id, char const* filename, uint16_t number_of_threads, uint32_t number_of_messages)
{
  // Start the logging backend thread
  quill::start();

  std::vector<std::thread> threads;

  for (int i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back([filename, number_of_messages, test_id, i]() {
      // Set writing logging to a file
      quill::Handler* log_from_one_thread_file = quill::filehandler(filename, "w");

      std::string logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);
      quill::Logger* logger = quill::create_logger(logger_name, log_from_one_thread_file);

      // Change the LogLevel to print everything
      logger->set_log_level(quill::LogLevel::TraceL3);

      for (int j = 0; j < number_of_messages; ++j)
      {
        LOG_INFO(logger, "Hello from thread {} this is message {}", i, j);
      }
    });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // Flush all log
  quill::flush();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  EXPECT_EQ(file_contents.size(), number_of_messages * number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    std::string expected_logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);

    for (int j = 0; j < number_of_messages; ++j)
    {
      std::string expected_string = expected_logger_name + " - " + "Hello from thread " +
                                    std::to_string(i) + " this is message " + std::to_string(j);

      EXPECT_TRUE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  std::remove(filename); // delete file
}

/***/
TEST(Quill, log_from_one_thread)
{
  static constexpr char const* filename = "log_from_one_thread.log";
  static constexpr size_t number_of_messages = 10000u;
  static constexpr size_t number_of_threads = 1;
  static constexpr char const* test_id = "single";

  test_quil_log(test_id, filename, number_of_threads, number_of_messages);
}

/***/
TEST(Quill, log_from_multiple_threads)
{
  static constexpr char const* filename = "log_from_multiple_threads.log";
  static constexpr size_t number_of_messages = 2000u;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* test_id = "multi";

  test_quil_log(test_id, filename, number_of_threads, number_of_messages);
}

/**
 * A test class example that is using the logger
 */
class log_test_class
{
public:
  log_test_class(char const* filename)
  {
    // create a new logger in the ctor
    quill::Handler* filehandler = quill::filehandler(filename, "w");
    _logger = quill::create_logger("test_class", filehandler);
  }

  /**
   * Use logger in const function
   */
  void use_logger_const() const noexcept
  {
    LOG_INFO(_logger, "Test message for test class const");
  }

  /**
   * Use logger in normal function
   */
  void use_logger()
  {
    LOG_INFO(_logger, "Test message for test class non const");
  }
private:
  quill::Logger* _logger{ nullptr};
};

/***/
TEST(Quill, log_from_const_function)
{
  static constexpr char const* filename = "log_test_class.log";
  // log for class a
  log_test_class log_test_class_a {filename};
  log_test_class_a.use_logger_const();
  log_test_class_a.use_logger();

  // log again for class b
  log_test_class const log_test_class_b {filename};
  log_test_class_b.use_logger_const();

  quill::flush();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  EXPECT_EQ(file_contents.size(), 3);

  std::remove(filename);
}