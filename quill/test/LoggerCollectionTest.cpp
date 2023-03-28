#include "doctest/doctest.h"

#include "quill/Config.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/handlers/StreamHandler.h"

TEST_SUITE_BEGIN("LoggerCollection");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("create_get_same_logger")
{
  // Create and then get the same logger and check that the values we set are cached
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  auto stream_handler = hc.stdout_console_handler();
  Logger* logger_1 =
    logger_collection.create_logger("logger_1", stream_handler, TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* logger_2 = logger_collection.get_logger("logger_1");
  REQUIRE_EQ(logger_2->log_level(), LogLevel::TraceL2);
}

/***/
TEST_CASE("create_get_all_loggers")
{
  // Create and then get the same logger and check that the values we set are cached
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  auto stream_handler = hc.stdout_console_handler();
  Logger* logger_1 =
    logger_collection.create_logger("logger_1", stream_handler, TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(logger_1->log_level(), LogLevel::Info);
  logger_1->set_log_level(LogLevel::TraceL2);
  REQUIRE_EQ(logger_1->log_level(), LogLevel::TraceL2);

  Logger* logger_2 =
    logger_collection.create_logger("logger_2", stream_handler, TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(logger_2->log_level(), LogLevel::Info);
  logger_2->set_log_level(LogLevel::Debug);
  REQUIRE_EQ(logger_2->log_level(), LogLevel::Debug);

  Logger* logger_3 =
    logger_collection.create_logger("logger_3", stream_handler, TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(logger_3->log_level(), LogLevel::Info);
  logger_3->set_log_level(LogLevel::Error);
  REQUIRE_EQ(logger_3->log_level(), LogLevel::Error);

  // Get all created loggers
  std::unordered_map<std::string, Logger*> all_loggers = logger_collection.get_all_loggers();
  REQUIRE_EQ(all_loggers.size(), 4);
  REQUIRE_EQ(logger_collection.get_all_loggers().find("root")->second->log_level(), LogLevel::Info);
  REQUIRE_EQ(logger_collection.get_all_loggers().find("logger_1")->second->log_level(), LogLevel::TraceL2);
  REQUIRE_EQ(logger_collection.get_all_loggers().find("logger_2")->second->log_level(), LogLevel::Debug);
  REQUIRE_EQ(logger_collection.get_all_loggers().find("logger_3")->second->log_level(), LogLevel::Error);
}

/***/
TEST_CASE("create_get_different_loggers")
{
  // Create and then get the same logger and check that the values we set are different
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  auto stream_handler = hc.stdout_console_handler();
  Logger* logger_1 =
    logger_collection.create_logger("logger_1", stream_handler, TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get a new logger with a default log level
  auto stream_handler_2 = hc.stdout_console_handler();
  QUILL_MAYBE_UNUSED Logger* logger_2 =
    logger_collection.create_logger("logger_2", stream_handler_2, TimestampClockType::Tsc, nullptr);
  Logger* logger_3 = logger_collection.get_logger("logger_2");
  REQUIRE_EQ(logger_3->log_level(), LogLevel::Info);
}

/***/
TEST_CASE("get_non_existing_logger")
{
  // Check that we throw if we try to get a logger that was never created before
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  // try to get a new logger with a default log level
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(QUILL_MAYBE_UNUSED auto logger = logger_collection.get_logger("logger_2"), quill::QuillError);
#endif
}

/***/
TEST_CASE("root_logger")
{
  // Get the root logger and change the log level, then get the root logger again and check
  // the values we set are cached
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  Logger* default_logger = logger_collection.get_logger();
  REQUIRE_EQ(default_logger->log_level(), LogLevel::Info);

  // change existing log level
  default_logger->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* default_logger_2 = logger_collection.get_logger();
  REQUIRE_EQ(default_logger_2->log_level(), LogLevel::TraceL2);

  // Get the root logger by name
  Logger* default_logger_3 = logger_collection.get_logger("root");
  REQUIRE_EQ(default_logger_3->log_level(), LogLevel::TraceL2);
}

/***/
TEST_CASE("create_logger_from_default_logger")
{
  // Create a new logger and check that the properties are the same as the root logger
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{cfg, tc, hc};

  Logger* default_logger = logger_collection.create_logger("logger_test", TimestampClockType::Tsc, nullptr);
  REQUIRE_EQ(default_logger->log_level(), LogLevel::Info);
}

TEST_SUITE_END();