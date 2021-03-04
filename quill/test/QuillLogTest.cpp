#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Quill.h"
#include "quill/detail/misc/FileUtilities.h"
#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("QuillLog");

// Note: This thread is flushing using the main() gtest test thread. This means that no-other test should have used flush()
// on the main gtest test thread as the main thread's thread context is not re-added.
// Other tests use flush() but within their spawned threads.
// this is never an issue in real logger as everything goes through the singleton, but we are not using the
// singleton all the time during testing

void test_quill_log(char const* test_id, std::string const& filename, uint16_t number_of_threads,
                    uint32_t number_of_messages)
{
  // Start the logging backend thread
  quill::start();

  std::vector<std::thread> threads;

  for (int i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back([filename, number_of_messages, test_id, i]() {
      // Also use preallocate
      quill::preallocate();

      // Set writing logging to a file
      quill::Handler* log_from_one_thread_file = quill::file_handler(filename, "w");

      std::string logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);
      quill::Logger* logger = quill::create_logger(logger_name.data(), log_from_one_thread_file);

      // Change the LogLevel to print everything
      logger->set_log_level(quill::LogLevel::TraceL3);

      for (uint32_t j = 0; j < number_of_messages; ++j)
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

#if defined(_WIN32)
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(quill::detail::s2ws(filename));
#else
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
#endif

  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads);

  for (int i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    std::string expected_logger_name = "logger_" + std::string{test_id} + "_" + std::to_string(i);

    for (uint32_t j = 0; j < number_of_messages; ++j)
    {
      std::string expected_string = expected_logger_name + " - " + "Hello from thread " +
        std::to_string(i) + " this is message " + std::to_string(j);

      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

#if defined(_WIN32)
  quill::detail::file_utilities::remove(quill::detail::s2ws(filename));
#else
  quill::detail::file_utilities::remove(filename);
#endif
}

/***/
TEST_CASE("log_from_one_thread")
{
  static constexpr size_t number_of_messages = 10000u;
  static constexpr size_t number_of_threads = 1;
  static constexpr char const* test_id = "single";

#if defined(_WIN32)
  static constexpr wchar_t const* filename = L"log_from_one_thread.log";
  test_quill_log(test_id, quill::detail::ws2s(filename), number_of_threads, number_of_messages);
#else
  static constexpr char const* filename = "log_from_one_thread.log";
  test_quill_log(test_id, filename, number_of_threads, number_of_messages);
#endif
}

/***/
TEST_CASE("log_from_multiple_threads")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 10;
  static constexpr char const* test_id = "multi";

#if defined(_WIN32)
  static constexpr wchar_t const* filename = L"log_from_multiple_threads.log";
  test_quill_log(test_id, quill::detail::ws2s(filename), number_of_threads, number_of_messages);
#else
  static constexpr char const* filename = "log_from_multiple_threads.log";
  test_quill_log(test_id, filename, number_of_threads, number_of_messages);
#endif
}

/**
 * A test class example that is using the logger
 */
class log_test_class
{
public:
  explicit log_test_class(std::string const& filename, quill::FilenameAppend filename_append)
  {
    // create a new logger in the ctor
    quill::Handler* filehandler = quill::file_handler(filename, "w", filename_append);
    _logger = quill::create_logger("test_class", filehandler);
  }

  /**
   * Use logger in const function
   */
  void use_logger_const() const noexcept { LOG_INFO(_logger, "Test message for test class const"); }

  /**
   * Use logger in normal function
   */
  void use_logger() { LOG_INFO(_logger, "Test message for test class non const"); }

private:
  quill::Logger* _logger{nullptr};
};

/***/
TEST_CASE("log_from_const_function")
{
  static constexpr char const* filename = "log_test_class.log";

  // Start the logging backend thread
  quill::start();

  // log for class a
  log_test_class log_test_class_a{filename, quill::FilenameAppend::None};
  log_test_class_a.use_logger_const();
  log_test_class_a.use_logger();

  // log again for class b
  log_test_class const log_test_class_b{filename, quill::FilenameAppend::None};
  log_test_class_b.use_logger_const();

  quill::flush();

#if defined(_WIN32)
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(quill::detail::s2ws(filename));
  REQUIRE_EQ(file_contents.size(), 3);
  quill::detail::file_utilities::remove(quill::detail::s2ws(filename));
#else
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 3);
  quill::detail::file_utilities::remove(filename);
#endif
}

/***/
TEST_CASE("log_using_rotating_file_handler")
{
  // This test has 2 loggers that they logged to different rotating file handlers

  static char const* base_filename = "rot_logger.log";
  static constexpr char const* rotated_filename_1 = "rot_logger.1.log";
  static constexpr char const* rotated_filename_2 = "rot_logger.2.log";
  static constexpr size_t max_file_size = 1024;

  // Start the logging backend thread
  quill::start();

  // Create a rotating file handler
  QUILL_MAYBE_UNUSED quill::Handler* rotating_file_handler =
    quill::rotating_file_handler(base_filename, "w", max_file_size, 2);

  // Get the same instance back - we search it again (for testing only)
  quill::Handler* looked_up_rotating_file_handler = quill::rotating_file_handler(base_filename);
  quill::Logger* rotating_logger = quill::create_logger("rot_logger", looked_up_rotating_file_handler);

  // Another rotating logger to another file with max backup count 1 this time. Here we rotate only once
  static char const* base_filename_2 = "rot_2nd_logger.log";
  static constexpr char const* rotated_filename_2nd_1 = "rot_2nd_logger.1.log";

  QUILL_MAYBE_UNUSED quill::Handler* rotating_file_handler_2 =
    quill::rotating_file_handler(base_filename_2, "w", max_file_size, 1);

  quill::Logger* rotating_logger_2 = quill::create_logger("rot_2nd_logger", rotating_file_handler_2);

  // log a few messages so we rotate files
  for (uint32_t i = 0; i < 20; ++i)
  {
    LOG_INFO(rotating_logger, "Hello rotating file log num {}", i);
    LOG_INFO(rotating_logger_2, "Hello rotating file log num {}", i);
  }

  quill::flush();

#if defined(_WIN32)
  // Read file and check
  std::vector<std::string> const file_contents =
    quill::testing::file_contents(quill::detail::s2ws(base_filename));
  REQUIRE_EQ(file_contents.size(), 4);

  std::vector<std::string> const file_contents_1 =
    quill::testing::file_contents(quill::detail::s2ws(rotated_filename_1));
  REQUIRE_EQ(file_contents_1.size(), 8);

  std::vector<std::string> const file_contents_2 =
    quill::testing::file_contents(quill::detail::s2ws(rotated_filename_2));
  REQUIRE_EQ(file_contents_2.size(), 8);

  std::vector<std::string> const file_contents_3 =
    quill::testing::file_contents(quill::detail::s2ws(base_filename_2));
  REQUIRE_EQ(file_contents_3.size(), 12);

  std::vector<std::string> const file_contents_4 =
    quill::testing::file_contents(quill::detail::s2ws(rotated_filename_2nd_1));
  REQUIRE_EQ(file_contents_4.size(), 8);
#else
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(base_filename);
  REQUIRE_EQ(file_contents.size(), 4);

  std::vector<std::string> const file_contents_1 = quill::testing::file_contents(rotated_filename_1);
  REQUIRE_EQ(file_contents_1.size(), 8);

  std::vector<std::string> const file_contents_2 = quill::testing::file_contents(rotated_filename_2);
  REQUIRE_EQ(file_contents_2.size(), 8);

  // File from 2nd logger
  std::vector<std::string> const file_contents_3 = quill::testing::file_contents(base_filename_2);
  REQUIRE_EQ(file_contents_3.size(), 12);

  std::vector<std::string> const file_contents_4 = quill::testing::file_contents(rotated_filename_2nd_1);
  REQUIRE_EQ(file_contents_4.size(), 8);
#endif

#if defined(_WIN32)
  // Remove filenames
  quill::detail::file_utilities::remove(quill::detail::s2ws(base_filename));
  quill::detail::file_utilities::remove(quill::detail::s2ws(rotated_filename_1));
  quill::detail::file_utilities::remove(quill::detail::s2ws(rotated_filename_2));
  quill::detail::file_utilities::remove(quill::detail::s2ws(base_filename_2));
  quill::detail::file_utilities::remove(quill::detail::s2ws(rotated_filename_2nd_1));
#else
  // Remove filenames
  quill::detail::file_utilities::remove(base_filename);
  quill::detail::file_utilities::remove(rotated_filename_1);
  quill::detail::file_utilities::remove(rotated_filename_2);
  quill::detail::file_utilities::remove(base_filename_2);
  quill::detail::file_utilities::remove(rotated_filename_2nd_1);
#endif
}

/***/
TEST_CASE("log_using_daily_file_handler")
{
  // This is not testing the daily rotation of the daily file logger
  static char const* base_filename = "log_daily.log";

  // Start the logging backend thread
  quill::start();

  // Create the handler
  QUILL_MAYBE_UNUSED quill::Handler* time_rotating_file_handler_create =
    quill::time_rotating_file_handler(base_filename, "w", "daily", 1, 0);

  // Get the same handler
  quill::Handler* time_rotating_file_handler = quill::time_rotating_file_handler(base_filename);

  quill::Logger* daily_logger = quill::create_logger("daily_logger", time_rotating_file_handler);

  // log a few messages
  for (uint32_t i = 0; i < 20; ++i)
  {
    LOG_INFO(daily_logger, "Hello daily file log num {}", i);
  }

  quill::flush();

#if defined(_WIN32)
  std::vector<std::string> const file_contents =
    quill::testing::file_contents(quill::detail::s2ws(base_filename));
  REQUIRE_EQ(file_contents.size(), 20);
#else
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(base_filename);
  REQUIRE_EQ(file_contents.size(), 20);
#endif

#if defined(_WIN32)
  // Remove filenames
  quill::detail::file_utilities::remove(quill::detail::s2ws(base_filename));
  quill::detail::file_utilities::remove(quill::detail::s2ws(base_filename));
  quill::detail::file_utilities::remove(quill::detail::s2ws(base_filename));
#else
  // Remove filenames
  quill::detail::file_utilities::remove(base_filename);
  quill::detail::file_utilities::remove(base_filename);
  quill::detail::file_utilities::remove(base_filename);
#endif
}

/***/
TEST_CASE("log_using_multiple_stdout_formats")
{
  // Tests the logging in stdcout and also multiple stdcout formats
  quill::start();

  quill::testing::CaptureStdout();

  quill::Handler* stdout_custom_handler = quill::stdout_handler("stdout_custom_1");
  stdout_custom_handler->set_pattern(
    QUILL_STRING("%(logger_name) - %(message) (%(function_name))"));
  quill::Logger* custom_logger = quill::create_logger("custom", stdout_custom_handler);

  // log a few messages so we rotate files
  for (uint32_t i = 0; i < 20; ++i)
  {
    if (i % 2 == 0)
    {
      LOG_INFO(quill::get_logger(), "Hello log num {}", i);
    }
    else
    {
      LOG_INFO(custom_logger, "Hello log num {}", i);
    }
  }

  quill::flush();

  // convert result to vector
  std::string results = quill::testing::GetCapturedStdout();
  std::stringstream data(results);

  std::string line;
  std::vector<std::string> result_arr;
  while (std::getline(data, line, '\n'))
  {
    result_arr.push_back(line);
  }

  for (uint32_t i = 0; i < 20; ++i)
  {
    if (i % 2 == 0)
    {
      std::string expected_string =
        "QuillLogTest.cpp:340         LOG_INFO      root         - Hello log num " + std::to_string(i);

      if (!quill::testing::file_contains(result_arr, expected_string))
      {
        FAIL(fmt::format("expected [{}] is not in results [{}]", expected_string, result_arr).data());
      }
    }
    else
    {

      std::string expected_string =
        "custom - Hello log num " + std::to_string(i) + " (_DOCTEST_ANON_FUNC_14)";

      if (!quill::testing::file_contains(result_arr, expected_string))
      {
        FAIL(fmt::format("expected [{}] is not in results [{}]", expected_string, result_arr).data());
      }
    }
  }
}

/***/
TEST_CASE("log_using_stderr")
{
  // Tests the logging in stdcout and also multiple stdcout formats. Also tests changing the default logger
  quill::start();

  quill::testing::CaptureStderr();

  quill::Handler* stderr_handler = quill::stderr_handler("stderr_custom_1");
  stderr_handler->set_pattern(QUILL_STRING("%(logger_name) - %(message) (%(function_name))"));
  quill::Logger* custom_logger = quill::create_logger("log_using_stderr", stderr_handler);

  LOG_INFO(custom_logger, "Hello log stderr");
  LOG_INFO(custom_logger, "Hello log stderr again");

  quill::flush();

  std::string results = quill::testing::GetCapturedStderr();

  REQUIRE_EQ(results,
             "log_using_stderr - Hello log stderr (_DOCTEST_ANON_FUNC_18)\n"
             "log_using_stderr - Hello log stderr again (_DOCTEST_ANON_FUNC_18)\n");
}

/***/
TEST_CASE("log_to_multiple_handlers_from_same_logger")
{
  // test logging to two or more handlers from the same logger
  quill::start();

  quill::testing::CaptureStderr();
  quill::testing::CaptureStdout();

  quill::Handler* stderr_handler = quill::stderr_handler();
  stderr_handler->set_pattern(QUILL_STRING("%(logger_name) - %(message) (%(function_name))"));

  quill::Handler* stdout_handler = quill::stdout_handler();
  stdout_handler->set_pattern(QUILL_STRING("%(logger_name) - %(message) (%(function_name))"));

  // Create the new logger with multiple handlers
  quill::Logger* custom_logger = quill::create_logger("log_multi_handlers", {stdout_handler, stderr_handler});

  LOG_INFO(custom_logger, "Hello log multiple handlers");

  quill::flush();

  std::string results_handler_1 = quill::testing::GetCapturedStderr();
  std::string results_handler_2 = quill::testing::GetCapturedStdout();

  REQUIRE_EQ(results_handler_1,
             "log_multi_handlers - Hello log multiple handlers (_DOCTEST_ANON_FUNC_20)\n");
  REQUIRE_EQ(results_handler_2,
             "log_multi_handlers - Hello log multiple handlers (_DOCTEST_ANON_FUNC_20)\n");
}

/***/
TEST_CASE("check_log_arguments_evaluation")
{
  size_t cnt{0};
  auto arg_str = [&cnt](){
    ++cnt;
    return "expensive_calculation";
  };

  static constexpr char const* filename = "check_log_arguments_evaluation.log";

  // create a new logger in the ctor
  quill::Handler* filehandler = quill::file_handler(filename, "w");
  auto logger = quill::create_logger("logger", filehandler);

  // Start the logging backend thread
  quill::start();

  // 1. checks that log arguments are not evaluated when we don't log
  logger->set_log_level(quill::LogLevel::Info);
  LOG_DEBUG(logger, "Test log arguments {}", arg_str());
  LOG_TRACE_L1(logger, "Test log arguments {}", arg_str());
  REQUIRE_EQ(cnt, 0);

  // 2. checks that log arguments are evaluated only once per log statement
  LOG_INFO(logger, "Test log arguments {}", arg_str());
  REQUIRE_EQ(cnt, 1);

  quill::flush();

#if defined(_WIN32)
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(quill::detail::s2ws(filename));
  REQUIRE_EQ(file_contents.size(), 1);
  quill::detail::file_utilities::remove(quill::detail::s2ws(filename));
#else
  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);
  quill::detail::file_utilities::remove(filename);
#endif
}
#ifndef QUILL_NO_EXCEPTIONS
/***/
TEST_CASE("invalid_handlers")
{
  quill::Handler* stderr_handler = quill::stderr_handler("stderr_handler");

  // using the same name again ast stdouthandler
  REQUIRE_THROWS_AS(auto x1 = quill::stdout_handler("stderr_handler"), quill::QuillError);

  quill::Handler* stdout_handler = quill::stdout_handler("stdout_handler");
  // using the same name again ast stdouthandler
  REQUIRE_THROWS_AS(auto x2 = quill::stderr_handler("stdout_handler"), quill::QuillError);

  static constexpr char const* filename = "invalid_handlers.log";
  quill::Handler* log_from_one_thread_file = quill::file_handler(filename, "w");
  // using the same handler again
  REQUIRE_THROWS_AS(auto x3 = quill::stderr_handler("invalid_handlers.log"), quill::QuillError);
  REQUIRE_THROWS_AS(auto x4 = quill::stdout_handler("invalid_handlers.log"), quill::QuillError);

  // try to use console colours with stdout handler as name
  quill::ConsoleColours terminal_colours;
  terminal_colours.set_default_colours();
  REQUIRE_THROWS_AS(auto x5 = quill::stdout_handler("stdout", terminal_colours), quill::QuillError);

  // remove file
  #if defined(_WIN32)
  quill::detail::file_utilities::remove(quill::detail::s2ws(filename));
  #else
  quill::detail::file_utilities::remove(filename);
  #endif
}
#endif

enum RawEnum : int
{
  Test1 = 1,
  Test2 = 2,
  Test3 = 3
};
std::ostream& operator<<(std::ostream& os, const RawEnum& raw_enum)
{
  switch (raw_enum) {
  case RawEnum::Test1:
    os << "Test1";
    break;
  case RawEnum::Test2:
    os << "Test2";
    break;
  case RawEnum::Test3:
    os << "Test3";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

enum class EnumClass : int
{
  Test4 = 4,
  Test5 = 5,
  Test6 = 6
};
std::ostream& operator<<(std::ostream& os, const EnumClass& enum_class)
{
  switch (enum_class) {
  case EnumClass::Test4:
    os << "Test4";
    break;
  case EnumClass::Test5:
    os << "Test5";
    break;
  case EnumClass::Test6:
    os << "Test6";
    break;
  default:
    os << "Unknown";
    break;
  }
  return os;
}

/***/
TEST_CASE("log_enums_with_overloaded_insertion_operator")
{
  quill::start();

  quill::testing::CaptureStdout();

  quill::Handler* stdout_handler = quill::stdout_handler();
  stdout_handler->set_pattern(QUILL_STRING("%(message)"));

  quill::Logger* custom_logger = quill::create_logger("enum_logger", stdout_handler);

  LOG_INFO(custom_logger, "{},{},{},{},{},{}", Test1, Test2, Test3, EnumClass::Test4, EnumClass::Test5, EnumClass::Test6);

  quill::flush();

  std::string results = quill::testing::GetCapturedStdout();

  REQUIRE_EQ(results, "Test1,Test2,Test3,Test4,Test5,Test6\n");
}

TEST_SUITE_END();