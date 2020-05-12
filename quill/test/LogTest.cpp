#include <gtest/gtest.h>

#define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3

#include "misc/TestUtilities.h"
#include "quill/detail/LogMacros.h"
#include "quill/detail/LogManager.h"
#include "quill/detail/misc/FileUtilities.h"
#include "quill/handlers/Handler.h"
#include <cstdio>
#include <thread>

/**
 * Contains tests that include frontend and backend thread testing
 * In real logger usage LogManager would be a singleton
 */
using namespace quill;
using namespace quill::detail;

// Important - Because LogManager each time creates a new ThreadContextCollection the following can happen
// 1st Test - Main thread creates static thread local inside ThreadContext
// 2nd Test - Main thread will not recreate the ThreadContext but the new LogManager instance
// is using a new ThreadContextCollection instance resulting in not being able to see the context

// Real logger is using a singleton so that is not an issue
// For the tests we will use threads so that the thread local is also destructed with the LogManager

/***/
TEST(Log, default_logger_with_filehandler)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename{L"test_default_logger_with_filehandler"};
#else
  std::string const filename{"test_default_logger_with_filehandler"};
#endif

  // Set a file handler as the custom logger handler and log to it
  lm.logger_collection().set_default_logger_handler(
    lm.handler_collection().file_handler(filename, "w"));

  lm.start_backend_worker();

  std::thread frontend([&lm]() {
    Logger* default_logger = lm.logger_collection().get_logger();

    LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(default_logger,
              "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    // Let all log get flushed to the file
    lm.flush();
  });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  EXPECT_EQ(file_contents.size(), 2);
  EXPECT_TRUE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO     root - Lorem ipsum dolor sit amet, consectetur adipiscing elit"}));
  EXPECT_TRUE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR    root - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  lm.stop_backend_worker();
  quill::detail::file_utilities::remove(filename);
}

void custom_default_logger_same_handler(int test_case = 0)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename{L"test_custom_default_logger_same_handler"};
#else
  std::string const filename{"test_custom_default_logger_same_handler"};
#endif

  // Set a file handler the custom logger handler and log to it
  Handler* file_handler = lm.handler_collection().file_handler(filename, "w");
  file_handler->set_pattern(
    QUILL_STRING("%(ascii_time) %(logger_name) - %(message) [%(logger_name)]"));
  lm.logger_collection().set_default_logger_handler(file_handler);

  // Start logging
  lm.start_backend_worker();

  if (test_case == 0)
  {
    // Add a second logger using the same file handler
    Handler* file_handler_2 = lm.handler_collection().file_handler(filename);
    QUILL_MAYBE_UNUSED Logger* logger_2 = lm.logger_collection().create_logger("custom_logger", file_handler_2);
  }
  else if (test_case == 1)
  {
    // Add the other logger by using the default logger params
    QUILL_MAYBE_UNUSED Logger* logger_2 = lm.logger_collection().create_logger("custom_logger");
  }
  // Thread for default pattern
  std::thread frontend_default([&lm]() {
    Logger* default_logger = lm.logger_collection().get_logger();

    LOG_INFO(default_logger, "Default Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(default_logger,
              "Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    lm.flush();
  });

  // Thread for custom pattern
  std::thread frontend_custom([&lm]() {
    Logger* logger_2 = lm.logger_collection().get_logger("custom_logger");

    LOG_INFO(logger_2, "Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(logger_2,
              "Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    lm.flush();
  });

  frontend_custom.join();
  frontend_default.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  EXPECT_EQ(file_contents.size(), 4);

  std::string const first_log_line_default =
    "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit";
  std::string const second_log_line_default =
    "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus "
    "ante";

  std::string const first_log_line_custom =
    "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit "
    "[custom_logger]";
  std::string const second_log_line_custom =
    "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante "
    "[custom_logger]";

  EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_default));
  EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_default));
  EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_custom));
  EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_custom));

  lm.stop_backend_worker();
  quill::detail::file_utilities::remove(filename);
}

/***/
TEST(Log, custom_default_logger_same_file) { custom_default_logger_same_handler(0); }

/***/
TEST(Log, custom_default_logger_same_file_from_default_logger)
{
  custom_default_logger_same_handler(1);
}

void test_custom_default_logger_multiple_handlers(int test_case)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename_1{L"test_custom_default_logger_multiple_handlers_1"};
  std::wstring const filename_2{L"test_custom_default_logger_multiple_handlers_2"};
#else
  std::string const filename_1{"test_custom_default_logger_multiple_handlers_1"};
  std::string const filename_2{"test_custom_default_logger_multiple_handlers_2"};
#endif

  // Set a file handler the custom logger handler and log to it

  // First handler
  Handler* file_handler_1 = lm.handler_collection().file_handler(filename_1, "w");
  file_handler_1->set_pattern(
    QUILL_STRING("%(ascii_time) %(logger_name) - %(message) [%(logger_name)]"));

  // Second handler with different pattern
  Handler* file_handler_2 = lm.handler_collection().file_handler(filename_2, "w");
  file_handler_2->set_pattern(QUILL_STRING("%(ascii_time) %(logger_name) - %(message)"),
                              "%D %H:%M:%S.%Qms");

  lm.logger_collection().set_default_logger_handler({file_handler_1, file_handler_2});

  // Start logging
  lm.start_backend_worker();

  if (test_case == 0)
  {
    // Add a second logger using the same file handler
    Handler* file_handler_a = lm.handler_collection().file_handler(filename_1);
    Handler* file_handler_b = lm.handler_collection().file_handler(filename_2);
    QUILL_MAYBE_UNUSED Logger* logger_2 =
      lm.logger_collection().create_logger("custom_logger", {file_handler_a, file_handler_b});
  }
  else if (test_case == 1)
  {
    // Add the second logger constructing it from the params of the default logger
    QUILL_MAYBE_UNUSED Logger* logger_2 = lm.logger_collection().create_logger("custom_logger");
  }

  // Thread for default pattern
  std::thread frontend_default([&lm]() {
    Logger* default_logger = lm.logger_collection().get_logger();

    LOG_INFO(default_logger, "Default Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(default_logger,
              "Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    lm.flush();
  });

  // Thread for custom pattern
  std::thread frontend_custom([&lm]() {
    Logger* logger_2 = lm.logger_collection().get_logger("custom_logger");

    LOG_INFO(logger_2, "Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(logger_2,
              "Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    lm.flush();
  });

  frontend_custom.join();
  frontend_default.join();

  {
    // Validate handler 1
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_1);

    EXPECT_EQ(file_contents.size(), 4);

    std::string const first_log_line_default =
      "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit [root]";
    std::string const second_log_line_default =
      "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante "
      "[root]";

    std::string const first_log_line_custom =
      "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit "
      "[custom_logger]";
    std::string const second_log_line_custom =
      "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus "
      "ante [custom_logger]";

    EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_default));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_default));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_custom));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_custom));
  }

  {
    // Validate handler 2
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename_2);

    EXPECT_EQ(file_contents.size(), 4);

    std::string const first_log_line_default =
      "root - Default Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    std::string const second_log_line_default =
      "root - Default Nulla tempus, libero at dignissim viverra, lectus libero finibus ante";

    std::string const first_log_line_custom =
      "custom_logger - Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit";
    std::string const second_log_line_custom =
      "custom_logger - Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus "
      "ante";

    EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_default));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_default));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, first_log_line_custom));
    EXPECT_TRUE(quill::testing::file_contains(file_contents, second_log_line_custom));
  }

  lm.stop_backend_worker();
  quill::detail::file_utilities::remove(filename_1);
  quill::detail::file_utilities::remove(filename_2);
}

/***/
TEST(Log, custom_default_logger_multiple_handlers)
{
  test_custom_default_logger_multiple_handlers(0);
}

/***/
TEST(Log, custom_default_logger_multiple_handlers_from_default_logger)
{
  test_custom_default_logger_multiple_handlers(1);
}

/***/
TEST(Log, many_loggers_multiple_threads)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename{L"test_many_loggers_multiple_threads"};
#else
  std::string const filename{"test_many_loggers_multiple_threads"};
#endif

  // Set a file handler as the custom logger handler and log to it
  lm.logger_collection().set_default_logger_handler(
    lm.handler_collection().file_handler(filename, "w"));

  lm.start_backend_worker();

  // Spawn many threads
  std::vector<std::thread> threads;
  static constexpr size_t thread_count = 100;
  static constexpr size_t message_count = 120;

  for (int i = 0; i < thread_count; ++i)
  {
    threads.emplace_back([&lm, i]() {
      // Create a logger in this thread
      std::string logger_name = "logger_" + std::to_string(i);
      Logger* logger = lm.logger_collection().create_logger(logger_name.data());

      for (int j = 0; j < message_count; ++j)
      {
        LOG_INFO(logger, "Hello from thread {} this is message {}", i, j);
      }

      // Let all log get flushed to the file
      lm.flush();
    });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  EXPECT_EQ(file_contents.size(), thread_count * message_count);

  for (int i = 0; i < thread_count; ++i)
  {
    // for each thread
    std::string expected_logger_name = "logger_" + std::to_string(i);

    for (int j = 0; j < message_count; ++j)
    {
      std::string expected_string = expected_logger_name + " - " + "Hello from thread " +
        std::to_string(i) + " this is message " + std::to_string(j);

      EXPECT_TRUE(quill::testing::file_contains(file_contents, expected_string));
    }
  }

  lm.stop_backend_worker();
  quill::detail::file_utilities::remove(filename);
}

#if defined(_WIN32)
/***/
TEST(Log, default_logger_with_filehandler_wide_chars)
{
  LogManager lm;

  #if defined(_WIN32)
  std::wstring const filename{L"test_default_logger_with_filehandler"};
  #else
  std::string const filename{"test_default_logger_with_filehandler"};
  #endif

  // Set a file handler as the custom logger handler and log to it
  lm.logger_collection().set_default_logger_handler(
    lm.handler_collection().file_handler(filename, "w"));

  lm.start_backend_worker();

  std::thread frontend([&lm]() {
    Logger* default_logger = lm.logger_collection().get_logger();

    std::wstring arg_1 = L"consectetur adipiscing elit";
    LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, {}", arg_1);
    wchar_t const* arg_2 = L"lectus libero finibus ante";
    LOG_ERROR(default_logger, "Nulla tempus, libero at dignissim viverra, {}", arg_2);

    // Let all log get flushed to the file
    lm.flush();
  });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  EXPECT_EQ(file_contents.size(), 2);
  EXPECT_TRUE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO     root - Lorem ipsum dolor sit amet, consectetur adipiscing elit"}));
  EXPECT_TRUE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR    root - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante"}));

  lm.stop_backend_worker();
  quill::detail::file_utilities::remove(filename);
}
#endif

/***/
TEST(Log, backend_error_handler)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename{L"test_backend_error_handler"};
#else
  std::string const filename{"test_backend_error_handler"};
#endif

  // counter to check our error handler was invoked
  size_t error_handler_invoked{0};

  std::thread frontend([&lm, &filename, &error_handler_invoked]() {
    // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
    lm.config().set_backend_thread_cpu_affinity(static_cast<uint16_t>(321312));

    // Set invalid thread name
    lm.config().set_backend_thread_name(
      "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_ut_"
      "labore_et_dolore_magna_aliqua");

    // Set a custom error handler to handler exceptions
    lm.set_backend_worker_error_handler(
      [&error_handler_invoked](std::string const& s) { ++error_handler_invoked; });

    // Set a file handler as the custom logger handler and log to it
    lm.logger_collection().set_default_logger_handler(
      lm.handler_collection().file_handler(filename, "w"));

    // Start backend worker
    lm.start_backend_worker();

    Logger* default_logger = lm.logger_collection().get_logger();

    LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(default_logger,
              "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    // Let all log get flushed to the file - this waits for the backend to actually start and
    // thy to do something
    lm.flush();
  });

  frontend.join();

  // Check our handler was invoked since either set_backend_thread_name or set_backend_thread_cpu_affinity should have failed
  ASSERT_NE(error_handler_invoked, 0);

  lm.stop_backend_worker();

  quill::detail::file_utilities::remove(filename);
}

/***/
TEST(Log, backend_error_handler_log_from_backend_thread)
{
  LogManager lm;

#if defined(_WIN32)
  std::wstring const filename{L"test_backend_error_handler_log_from_backend_thread"};
#else
  std::string const filename{"test_backend_error_handler_log_from_backend_thread"};
#endif

  // counter to check our error handler was invoked
  size_t error_handler_invoked{0};

  std::thread frontend([&lm, &filename, &error_handler_invoked]() {
    // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
    lm.config().set_backend_thread_cpu_affinity(static_cast<uint16_t>(321312));

    // Set invalid thread name
    lm.config().set_backend_thread_name(
      "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_ut_"
      "labore_et_dolore_magna_aliqua");

    // Set a file handler as the custom logger handler and log to it
    lm.logger_collection().set_default_logger_handler(
      lm.handler_collection().file_handler(filename, "w"));

    Logger* default_logger = lm.logger_collection().get_logger();

    // Set a custom error handler to handler exceptions
    lm.set_backend_worker_error_handler([&error_handler_invoked, default_logger, &lm](std::string const& s) {
      LOG_WARNING(default_logger, "error handler invoked");
      lm.flush(); // this will be called by the backend but do nothing
    });

    // Start backend worker
    lm.start_backend_worker();

    LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
    LOG_ERROR(default_logger,
              "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

    // Let all log get flushed to the file - this waits for the backend to actually start and
    // thy to do something
    lm.flush();
  });

  frontend.join();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  EXPECT_TRUE(
    quill::testing::file_contains(file_contents, "LOG_WARNING  root - error handler invoked"));
  EXPECT_EQ(file_contents.size(), 3);

  lm.stop_backend_worker();

  quill::detail::file_utilities::remove(filename);
}