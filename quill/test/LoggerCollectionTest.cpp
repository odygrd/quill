#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"

#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(LoggerCollection, same_logger)
{
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* logger_1 = logger_collection.get_logger("logger_1");
  EXPECT_EQ(logger_1->id(), 2);
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* logger_2 = logger_collection.get_logger("logger_1");
  EXPECT_EQ(logger_2->id(), 2);
  EXPECT_EQ(logger_2->log_level(), LogLevel::TraceL2);
}

/***/
TEST(LoggerCollection, different_loggers)
{
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* logger_1 = logger_collection.get_logger("logger_1");
  EXPECT_EQ(logger_1->id(), 2);
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get a new logger with a default log level
  Logger* logger_2 = logger_collection.get_logger("logger_2");
  EXPECT_EQ(logger_2->id(), 3);
  EXPECT_EQ(logger_2->log_level(), LogLevel::Info);
}

/***/
TEST(LoggerCollection, logger_id_to_logger_name)
{
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* default_logger = logger_collection.get_logger();
  Logger* logger_1 = logger_collection.get_logger("logger_1");
  Logger* logger_2 = logger_collection.get_logger("logger_2");

  EXPECT_STREQ(logger_collection.get_logger_name(default_logger->id()), "root");
  EXPECT_STREQ(logger_collection.get_logger_name(logger_1->id()), "logger_1");
  EXPECT_STREQ(logger_collection.get_logger_name(logger_2->id()), "logger_2");
  EXPECT_THROW([[maybe_unused]] auto res = logger_collection.get_logger_name(4), std::runtime_error);
}

/***/
TEST(LoggerCollection, default_logger)
{
  ThreadContextCollection tc;
  LoggerCollection logger_collection{tc};

  Logger* default_logger = logger_collection.get_logger();
  EXPECT_EQ(default_logger->id(), 1);
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);

  // change existing log level
  default_logger->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* default_logger_2 = logger_collection.get_logger();
  EXPECT_EQ(default_logger_2->id(), 1);
  EXPECT_EQ(default_logger_2->log_level(), LogLevel::TraceL2);

  // Get the default logger by name
  Logger* default_logger_3 = logger_collection.get_logger("root");
  EXPECT_EQ(default_logger_3->id(), 1);
  EXPECT_EQ(default_logger_3->log_level(), LogLevel::TraceL2);
}