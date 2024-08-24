#include "doctest/doctest.h"

#include "quill/Logger.h"
#include "quill/core/LoggerManager.h"

#include "quill/sinks/ConsoleSink.h"

TEST_SUITE_BEGIN("Logger");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("check_logger")
{
  std::shared_ptr<ConsoleSink> sink = std::make_unique<ConsoleSink>();

  LoggerManager& lm = LoggerManager::instance();

  std::vector<std::shared_ptr<Sink>> sinks;
  sinks.push_back(std::move(sink));

  Logger* logger_1 = static_cast<Logger*>(lm.create_or_get_logger<Logger>(
    "logger_1", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
    "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr));

  // Check default log level
  REQUIRE_EQ(logger_1->get_log_level(), LogLevel::Info);
  REQUIRE_EQ(logger_1->get_logger_name(), "logger_1");

#ifndef QUILL_NO_EXCEPTIONS
  // throw if backtrace log level is used
  REQUIRE_THROWS_AS(logger_1->set_log_level(LogLevel::Backtrace), quill::QuillError);
#endif
}

/***/
TEST_CASE("logger_should_log")
{
  std::shared_ptr<ConsoleSink> sink = std::make_unique<ConsoleSink>();

  LoggerManager& lm = LoggerManager::instance();

  std::vector<std::shared_ptr<Sink>> sinks;
  sinks.push_back(std::move(sink));

  Logger* logger_1 = static_cast<Logger*>(lm.create_or_get_logger<Logger>(
    "logger_1", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
    "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr));

  REQUIRE_UNARY_FALSE(logger_1->should_log_statement<LogLevel::Debug>());
  REQUIRE(logger_1->should_log_statement<LogLevel::Info>());
  REQUIRE(logger_1->should_log_statement<LogLevel::Error>());

  // change log level
  logger_1->set_log_level(LogLevel::TraceL3);
  REQUIRE(logger_1->should_log_statement<LogLevel::TraceL3>());
  REQUIRE(logger_1->should_log_statement<LogLevel::Critical>());

  // change log level
  logger_1->set_log_level(LogLevel::None);
  REQUIRE_UNARY_FALSE(logger_1->should_log_statement<LogLevel::TraceL3>());
  REQUIRE_UNARY_FALSE(logger_1->should_log_statement<LogLevel::Critical>());
}

TEST_SUITE_END();
