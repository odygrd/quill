#include "doctest/doctest.h"

#include "quill/Logger.h"
#include "quill/core/LoggerManager.h"
#include "quill/sinks/ConsoleSink.h"

TEST_SUITE_BEGIN("LoggerManager");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("create_get_remove_logger")
{
  std::shared_ptr<ConsoleSink> sink = std::make_unique<ConsoleSink>();

  LoggerManager& lm = LoggerManager::instance();

  std::vector<std::shared_ptr<Sink>> sinks;
  sinks.push_back(sink);

  LoggerBase* logger_1 = lm.create_or_get_logger<Logger>(
    "logger_1", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  LoggerBase* logger_2 = lm.create_or_get_logger<Logger>(
    "logger_2", std::initializer_list<std::shared_ptr<Sink>>{sink},
    PatternFormatterOptions{"[%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  REQUIRE_EQ(logger_1->get_logger_name(), "logger_1");
  REQUIRE_EQ(logger_2->get_logger_name(), "logger_2");

  REQUIRE_EQ(lm.get_logger("logger_1")->get_logger_name(), "logger_1");
  REQUIRE_EQ(lm.get_logger("logger_2")->get_logger_name(), "logger_2");
  REQUIRE_EQ(lm.get_logger("logger_3"), nullptr);

  REQUIRE_EQ(lm.get_all_loggers().size(), 2);
  REQUIRE_EQ(lm.get_all_loggers()[0]->get_logger_name(), "logger_1");
  REQUIRE_EQ(lm.get_all_loggers()[1]->get_logger_name(), "logger_2");

  // try_remove_logger_1_queue_has_pending_messages
  {
    lm.remove_logger(logger_1);

    // Logger is only marked invalid at this stage
    REQUIRE_EQ(lm.get_all_loggers().size(), 1);
    REQUIRE_EQ(lm.get_all_loggers()[0]->get_logger_name(), "logger_2");

    // count will include also the invalidated loggers
    REQUIRE_EQ(lm.get_number_of_loggers(), 2);

    // Logger is not removed yet because we have pending records
    std::vector<std::string> removed_loggers;
    lm.cleanup_invalidated_loggers([]() { return false; }, removed_loggers);

    REQUIRE_EQ(removed_loggers.size(), 0);
    REQUIRE_EQ(lm.get_number_of_loggers(), 2);
  }

  // try_remove_logger_1_queue_is_empty
  {
    lm.remove_logger(logger_1);

    // Logger is only marked invalid at this stage
    REQUIRE_EQ(lm.get_all_loggers().size(), 1);
    REQUIRE_EQ(lm.get_all_loggers()[0]->get_logger_name(), "logger_2");

    // count will include also the invalidated loggers
    REQUIRE_EQ(lm.get_number_of_loggers(), 2);

    // Logger is removed since we pass true meaning the queue is empty
    std::vector<std::string> removed_loggers;
    lm.cleanup_invalidated_loggers([]() { return true; }, removed_loggers);

    REQUIRE_EQ(removed_loggers.size(), 1);
    REQUIRE_EQ(removed_loggers[0], "logger_1");

    REQUIRE_EQ(lm.get_all_loggers().size(), 1);
    REQUIRE_EQ(lm.get_all_loggers()[0]->get_logger_name(), "logger_2");

    // Number of loggers is also updated after removal
    REQUIRE_EQ(lm.get_number_of_loggers(), 1);
  }

  // try_remove_logger_2_queue_has_pending_messages
  {
    lm.remove_logger(logger_2);

    // Logger is only marked invalid at this stage
    REQUIRE_EQ(lm.get_all_loggers().size(), 0);

    // Logger is not removed yet because we have pending records
    std::vector<std::string> removed_loggers;
    lm.cleanup_invalidated_loggers([]() { return false; }, removed_loggers);

    REQUIRE_EQ(removed_loggers.size(), 0);

    REQUIRE_EQ(lm.get_all_loggers().size(), 0);
    REQUIRE_EQ(lm.get_number_of_loggers(), 1);
  }

  // try_remove_logger_2_queue_is_empty
  {
    lm.remove_logger(logger_2);

    // Logger is only marked invalid at this stage
    REQUIRE_EQ(lm.get_all_loggers().size(), 0);

    // Logger is removed since we pass true meaning the queue is empty
    std::vector<std::string> removed_loggers;
    lm.cleanup_invalidated_loggers([]() { return true; }, removed_loggers);

    REQUIRE_EQ(removed_loggers.size(), 1);
    REQUIRE_EQ(removed_loggers[0], "logger_2");

    REQUIRE_EQ(lm.get_all_loggers().size(), 0);
    REQUIRE_EQ(lm.get_number_of_loggers(), 0);
  }

  REQUIRE_EQ(lm.get_valid_logger(), nullptr);
}

/***/
TEST_CASE("get_valid_logger_with_exclusions")
{
  std::shared_ptr<ConsoleSink> sink = std::make_unique<ConsoleSink>();

  LoggerManager& lm = LoggerManager::instance();

  std::vector<std::shared_ptr<Sink>> sinks;
  sinks.push_back(sink);

  // Create various loggers with different naming patterns
  // Note: Loggers are stored in alphabetical order
  LoggerBase* logger_normal = lm.create_or_get_logger<Logger>(
    "logger_normal", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  sinks.clear();
  sinks.push_back(sink);
  LoggerBase* logger_csv = lm.create_or_get_logger<Logger>(
    "logger__csv__output", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  sinks.clear();
  sinks.push_back(sink);
  LoggerBase* logger_binary = lm.create_or_get_logger<Logger>(
    "logger__binary__data", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  sinks.clear();
  sinks.push_back(sink);
  LoggerBase* logger_debug = lm.create_or_get_logger<Logger>(
    "logger__debug__trace", std::move(sinks),
    PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                            "LOG_%(log_level:<9) %(logger:<12) %(message)",
                            "%H:%M:%S.%Qns", quill::Timezone::GmtTime, false},
    ClockSourceType::Tsc, nullptr);

  REQUIRE_EQ(lm.get_all_loggers().size(), 4);

  // Alphabetically sorted order: logger__binary__data, logger__csv__output, logger__debug__trace, logger_normal

  {
    LoggerBase* valid_logger = lm.get_valid_logger();
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__binary__data");
  }

  {
    LoggerBase* valid_logger = lm.get_valid_logger("__binary__");
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__csv__output");
  }

  {
    LoggerBase* valid_logger = lm.get_valid_logger("__csv__");
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__binary__data");
  }

  {
    std::vector<std::string> exclusions{"__csv__", "__binary__"};
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__debug__trace");
  }

  {
    std::vector<std::string> exclusions{"normal", "__debug__"};
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__binary__data");
  }

  {
    std::vector<std::string> exclusions{"__csv__", "__binary__", "__debug__"};
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger_normal");
  }

  {
    std::vector<std::string> exclusions{"logger"};
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_EQ(valid_logger, nullptr);
  }

  {
    std::vector<std::string> exclusions;
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__binary__data");
  }

  {
    lm.remove_logger(logger_binary);
    std::vector<std::string> removed_loggers;
    lm.cleanup_invalidated_loggers([]() { return true; }, removed_loggers);

    REQUIRE_EQ(removed_loggers.size(), 1);
    REQUIRE_EQ(removed_loggers[0], "logger__binary__data");
    REQUIRE_EQ(lm.get_all_loggers().size(), 3);

    LoggerBase* valid_logger = lm.get_valid_logger();
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__csv__output");

    valid_logger = lm.get_valid_logger("__csv__");
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__debug__trace");

    std::vector<std::string> exclusions{"__csv__", "__debug__"};
    valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger_normal");
  }

  {
    std::vector<std::string> exclusions{"", "__csv__", "__debug__"};
    LoggerBase* valid_logger = lm.get_valid_logger(exclusions);
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger_normal");
  }

  {
    LoggerBase* valid_logger = lm.get_valid_logger("__debug__");
    REQUIRE_NE(valid_logger, nullptr);
    REQUIRE_EQ(valid_logger->get_logger_name(), "logger__csv__output");
  }

  lm.remove_logger(logger_csv);
  lm.remove_logger(logger_normal);
  lm.remove_logger(logger_debug);

  std::vector<std::string> removed_loggers;
  lm.cleanup_invalidated_loggers([]() { return true; }, removed_loggers);

  REQUIRE_EQ(removed_loggers.size(), 3);
  REQUIRE_EQ(lm.get_all_loggers().size(), 0);
  REQUIRE_EQ(lm.get_number_of_loggers(), 0);
  REQUIRE_EQ(lm.get_valid_logger(), nullptr);

  std::vector<std::string> exclusions{"__csv__"};
  REQUIRE_EQ(lm.get_valid_logger(exclusions), nullptr);
}

void set_env(char const* name, char const* value)
{
#if defined(_WIN32)
  std::string env_entry = std::string(name) + "=" + value;
  _putenv(env_entry.c_str());
#else
  setenv(name, value, 1); // 1 to overwrite
#endif
}

/***/
TEST_CASE("parse_log_level_from_env")
{
  LoggerManager& lm = LoggerManager::instance();

  // no env
  REQUIRE_EQ(lm.create_or_get_logger<Logger>("lg1", std::vector<std::shared_ptr<Sink>>{},
                                             PatternFormatterOptions{}, ClockSourceType::Tsc, nullptr)
               ->get_log_level(),
             quill::LogLevel::Info);

  set_env("QUILL_LOG_LEVEL", "tracel3");
  lm.parse_log_level_from_env();
  REQUIRE_EQ(lm.create_or_get_logger<Logger>("lg2", std::vector<std::shared_ptr<Sink>>{},
                                             PatternFormatterOptions{}, ClockSourceType::Tsc, nullptr)
               ->get_log_level(),
             quill::LogLevel::TraceL3);

  set_env("QUILL_LOG_LEVEL", "debug");
  lm.parse_log_level_from_env();
  REQUIRE_EQ(lm.create_or_get_logger<Logger>("lg3", std::vector<std::shared_ptr<Sink>>{},
                                             PatternFormatterOptions{}, ClockSourceType::Tsc, nullptr)
               ->get_log_level(),
             quill::LogLevel::Debug);

  set_env("QUILL_LOG_LEVEL", "critical");
  lm.parse_log_level_from_env();
  REQUIRE_EQ(lm.create_or_get_logger<Logger>("lg4", std::vector<std::shared_ptr<Sink>>{},
                                             PatternFormatterOptions{}, ClockSourceType::Tsc, nullptr)
               ->get_log_level(),
             quill::LogLevel::Critical);

  set_env("QUILL_LOG_LEVEL", "none");
  lm.parse_log_level_from_env();
  REQUIRE_EQ(lm.create_or_get_logger<Logger>("lg5", std::vector<std::shared_ptr<Sink>>{},
                                             PatternFormatterOptions{}, ClockSourceType::Tsc, nullptr)
               ->get_log_level(),
             quill::LogLevel::None);
}

TEST_SUITE_END();