#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/sinks/StdoutSink.h"

#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(LoggerCollection, same_logger)
{
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
TEST(LoggerCollection, different_loggers)
{
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
TEST(LoggerCollection, logger_not_found)
{
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  // try to get a new logger with a default log level
  EXPECT_THROW([[maybe_unused]] auto logger = logger_collection.get_logger("logger_2"), std::runtime_error);
}

/***/
TEST(LoggerCollection, default_logger)
{
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
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* default_logger = logger_collection.create_logger("logger_test");
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);
}