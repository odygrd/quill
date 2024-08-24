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
    std::vector<std::string> const removed_loggers =
      lm.cleanup_invalidated_loggers([]() { return false; });

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
    std::vector<std::string> const removed_loggers =
      lm.cleanup_invalidated_loggers([]() { return true; });

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
    std::vector<std::string> const removed_loggers =
      lm.cleanup_invalidated_loggers([]() { return false; });

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
    std::vector<std::string> const removed_loggers =
      lm.cleanup_invalidated_loggers([]() { return true; });

    REQUIRE_EQ(removed_loggers.size(), 1);
    REQUIRE_EQ(removed_loggers[0], "logger_2");

    REQUIRE_EQ(lm.get_all_loggers().size(), 0);
    REQUIRE_EQ(lm.get_number_of_loggers(), 0);
  }

  REQUIRE_EQ(lm.get_valid_logger(), nullptr);
}

TEST_SUITE_END();