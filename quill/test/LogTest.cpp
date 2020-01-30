#include <gtest/gtest.h>

#define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3

#include "TestUtility.h"
#include "quill/Macros.h"
#include "quill/detail/LogManager.h"
#include "quill/handlers/Handler.h"
#include <thread>

/**
 * Contains tests that include frontend and backend thread testing
 * In real logger usage LogManager would be a singleton
 */
using namespace quill;
using namespace quill::detail;

static Config default_cfg;

// Important - Because LogManager each time creates a new ThreadContextCollection the following can happen
// 1st Test - Main thread creates static thread local inside ThreadContext
// 2nd Test - Main thread will not recreate the ThreadContext but the new LogManager instance
// is using a new ThreadContextCollection instance resulting in not being able to see the context

// Real logger is using a singleton so that is not an issue
// For the tests we will use threads so that the thread local is also destructed with the LogManager

TEST(Log, custom_default_logger)
{
  LogManager lm{default_cfg};

  std::string const filename{"test_custom_default_logger"};

  // Set a file handler as the custom logger handler and log to it
  lm.logger_collection().set_default_logger_handler(
    lm.handler_collection().filehandler(filename, "w"));

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

  std::string const file_contents = quill::testing::file_contents(filename);

  std::string const first_log_line =
    "LOG_INFO     root - Lorem ipsum dolor sit amet, consectetur adipiscing elit";
  std::string const second_log_line =
    "LOG_ERROR    root - Nulla tempus, libero at dignissim viverra, lectus libero finibus ante";

  EXPECT_TRUE(file_contents.find(first_log_line) != std::string::npos);
  EXPECT_TRUE(file_contents.find(second_log_line) != std::string::npos);

  lm.stop_backend_worker();
}

TEST(Log, custom_pattern_default_logger)
{
  LogManager lm{default_cfg};

  std::string const filename{"test_custom_pattern_default_logger"};

  // Set a file handler as the custom logger handler and a custom formatter pattern
  Handler* file_handler = lm.handler_collection().filehandler(filename, "w");
  file_handler->set_formatter(PatternFormatter{QUILL_STRING("%(message) [%(function_name)]")});

  lm.logger_collection().set_default_logger_handler(file_handler);

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

  std::string const file_contents = quill::testing::file_contents(filename);

  std::string const first_log_line =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit [operator()]";
  std::string const second_log_line =
    "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [operator()]";

  EXPECT_TRUE(file_contents.find(first_log_line) != std::string::npos);
  EXPECT_TRUE(file_contents.find(second_log_line) != std::string::npos);

  lm.stop_backend_worker();
}

TEST(Log, custom_pattern_logger_and_default_logger_same_file)
{
  LogManager lm{default_cfg};

  std::string const filename{"test_custom_pattern_logger_and_default_logger"};

  // Set a file handler the custom logger handler and log to it
  Handler* file_handler = lm.handler_collection().filehandler(filename, "w");
  lm.logger_collection().set_default_logger_handler(file_handler);

  // Start logging
  lm.start_backend_worker();

  // Add a second logger with a different pattern
  Handler* file_handler_2 = lm.handler_collection().filehandler(filename);
  file_handler_2->set_formatter(
    PatternFormatter{QUILL_STRING("%(ascii_time) %(message) [%(function_name)]")});
  [[maybe_unused]] Logger* logger_2 = lm.logger_collection().create_logger("custom_logger", file_handler_2);

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

  std::this_thread::sleep_for(std::chrono::microseconds{100});

  std::string const file_contents = quill::testing::file_contents(filename);

  std::string const first_log_line_default =
    "Default Lorem ipsum dolor sit amet, consectetur adipiscing elit";
  std::string const second_log_line_default =
    "Default Nulla tempus, libero at dignissim viverra, lectus libero finibus "
    "ante";

  std::string const first_log_line_custom =
    "Custom Lorem ipsum dolor sit amet, consectetur adipiscing elit [operator()]";
  std::string const second_log_line_custom =
    "Custom Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [operator()]";

  EXPECT_TRUE(file_contents.find(first_log_line_default) != std::string::npos);
  EXPECT_TRUE(file_contents.find(first_log_line_default) != std::string::npos);
  EXPECT_TRUE(file_contents.find(first_log_line_custom) != std::string::npos);
  EXPECT_TRUE(file_contents.find(second_log_line_custom) != std::string::npos);
}
// TODO :: file_contents to return vector

// TODO :: Add multiple handler

// TODO:: Add multiple handler, custom format

// TODO:: Add stress test