#include "doctest/doctest.h"

#include "quill/Config.h"
#include "quill/detail//HandlerCollection.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/handlers/Handler.h"

TEST_SUITE_BEGIN("Logger");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("log_level")
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{cfg, tc, hc};

  Logger* logger = logger_collection.create_logger("logger_1", TimestampClockType::Tsc, nullptr);

  // Check default log level
  REQUIRE_EQ(logger->log_level(), LogLevel::Info);

  // Change the log level
  logger->set_log_level(LogLevel::TraceL2);
  REQUIRE_EQ(logger->log_level(), LogLevel::TraceL2);
}

#ifndef QUILL_NO_EXCEPTIONS
/***/
TEST_CASE("get_non_existent_logger")
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{cfg, tc, hc};

  REQUIRE_THROWS_AS((void)logger_collection.get_logger("logger_1"), quill::QuillError);
}

/***/
TEST_CASE("throw_if_backtrace_log_level_is_used")
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{cfg, tc, hc};

  Logger* logger_1 = logger_collection.create_logger("logger_1", TimestampClockType::Tsc, nullptr);

  REQUIRE_THROWS_AS(logger_1->set_log_level(LogLevel::Backtrace), quill::QuillError);
}
#endif

/***/
TEST_CASE("logger_should_log")
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{cfg, tc, hc};

  QUILL_MAYBE_UNUSED Logger* logger_1 =
    logger_collection.create_logger("logger_1", TimestampClockType::Tsc, nullptr);

  REQUIRE_UNARY_FALSE(logger_collection.get_logger("logger_1")->should_log<LogLevel::Debug>());
  REQUIRE(logger_collection.get_logger("logger_1")->should_log<LogLevel::Info>());
  REQUIRE(logger_collection.get_logger("logger_1")->should_log<LogLevel::Error>());

  // change log level
  logger_collection.get_logger("logger_1")->set_log_level(LogLevel::TraceL3);
  REQUIRE(logger_collection.get_logger("logger_1")->should_log<LogLevel::TraceL3>());
  REQUIRE(logger_collection.get_logger("logger_1")->should_log<LogLevel::Critical>());

  // change log level
  logger_collection.get_logger("logger_1")->set_log_level(LogLevel::None);
  REQUIRE_UNARY_FALSE(logger_collection.get_logger("logger_1")->should_log<LogLevel::TraceL3>());
  REQUIRE_UNARY_FALSE(logger_collection.get_logger("logger_1")->should_log<LogLevel::Critical>());
}

TEST_SUITE_END();