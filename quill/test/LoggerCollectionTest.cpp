#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/sinks/StdoutSink.h"

#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(LoggerCollection, create_get_same_logger)
{
  // Create and then get the same logger and check that the values we set are cached
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  std::unique_ptr<SinkBase> sink = std::make_unique<StdoutSink>();
  Logger* logger_1 = logger_collection.create_logger("logger_1", std::move(sink));
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* logger_2 = logger_collection.get_logger("logger_1");
  EXPECT_EQ(logger_2->log_level(), LogLevel::TraceL2);
}

/***/
TEST(LoggerCollection, create_get_different_loggers)
{
  // Create and then get the same logger and check that the values we set are different
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* logger_1 = logger_collection.create_logger("logger_1", std::make_unique<StdoutSink>());
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get a new logger with a default log level
  [[maybe_unused]] Logger* logger_2 =
    logger_collection.create_logger("logger_2", std::make_unique<StdoutSink>());
  Logger* logger_3 = logger_collection.get_logger("logger_2");
  EXPECT_EQ(logger_3->log_level(), LogLevel::Info);
}

/***/
TEST(LoggerCollection, get_non_existing_logger)
{
  // Check that we throw if we try to get a logger that was never created before
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  // try to get a new logger with a default log level
  EXPECT_THROW([[maybe_unused]] auto logger = logger_collection.get_logger("logger_2"), std::runtime_error);
}

/***/
TEST(LoggerCollection, default_logger)
{
  // Get the default logger and change the log level, then get the default logger again ans check
  // the values we set are cached
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* default_logger = logger_collection.get_logger();
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);

  // change existing log level
  default_logger->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* default_logger_2 = logger_collection.get_logger();
  EXPECT_EQ(default_logger_2->log_level(), LogLevel::TraceL2);

  // Get the default logger by name
  Logger* default_logger_3 = logger_collection.get_logger("root");
  EXPECT_EQ(default_logger_3->log_level(), LogLevel::TraceL2);
}

/***/
TEST(LoggerCollection, create_logger_from_default_logger)
{
  // Create a new logger and check that the properties are the same as the default logger
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* default_logger = logger_collection.create_logger("logger_test");
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);
}

/***/
TEST(LoggerCollection, set_default_logger_sink)
{
  // Set a sink to the default logger
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  logger_collection.set_custom_default_logger(std::make_unique<StdoutSink>());
}

/***/
TEST(LoggerCollection, set_default_logger_pattern)
{
  // Set a sink to the default logger with a default pattern
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  logger_collection.set_custom_default_logger(
    std::make_unique<StdoutSink>(QUILL_STRING("%(message)")));
}

/***/
TEST(LoggerCollection, set_default_logger_multiple_sinks)
{
  // Set a sink to the default logger with a default pattern
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  logger_collection.set_custom_default_logger(std::make_unique<StdoutSink>(), std::make_unique<StdoutSink>());
}

/***/
TEST(LoggerCollection, set_default_logger_pattern_multiple_sinks)
{
  // Set a sink to the default logger with a default pattern
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  logger_collection.set_custom_default_logger(
    std::make_unique<StdoutSink>(QUILL_STRING("%(message)")),
    std::make_unique<StdoutSink>(QUILL_STRING("%(message)")));
}