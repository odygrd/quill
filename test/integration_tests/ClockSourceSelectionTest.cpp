#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/NullSink.h"

#include <atomic>
#include <memory>
#include <vector>

namespace
{
struct SelectionClock : quill::UserClockSource
{
  // ClockSource must bind the actual object without invoking an overloaded address-of operator.
  SelectionClock* operator&() noexcept { return nullptr; }

  uint64_t now() const override
  {
    reads.fetch_add(1, std::memory_order_relaxed);
    return 0;
  }

  mutable std::atomic<size_t> reads{0};
};
} // namespace

TEST_CASE("clock_source_selection")
{
  SelectionClock clock;
  auto sink = quill::Frontend::create_sink<quill::NullSink>("clock_source_selection_sink");
  quill::PatternFormatterOptions const pattern;

  // One configuration type supports choosing a source at runtime.
  quill::ClockSource selection = quill::ClockSource::tsc();
  quill::Logger* tsc_logger =
    quill::Frontend::create_logger("clock_source_selection_tsc", sink, pattern, selection);
  selection = quill::ClockSource::system();
  quill::Logger* system_logger = quill::Frontend::create_or_get_logger(
    "clock_source_selection_system", std::vector<std::shared_ptr<quill::Sink>>{sink}, pattern, selection);
  selection = quill::ClockSource::custom(clock);
  quill::Logger* user_logger =
    quill::Frontend::create_logger("clock_source_selection_user", {sink}, pattern, selection);

  CHECK_EQ(tsc_logger->get_clock_source_type(), quill::ClockSourceType::Tsc);
  CHECK_EQ(system_logger->get_clock_source_type(), quill::ClockSourceType::System);
  CHECK_EQ(user_logger->get_clock_source_type(), quill::ClockSourceType::User);
  CHECK_EQ(user_logger->get_user_clock_source(), std::addressof(clock));

  // Changing the configuration after creation does not change existing loggers.
  selection = quill::ClockSource::tsc();
  CHECK_EQ(quill::Frontend::create_or_get_logger("clock_source_selection_user", {sink}, pattern, selection),
           user_logger);
  CHECK_EQ(user_logger->get_user_clock_source(), std::addressof(clock));

  LOG_INFO(tsc_logger, "built-in TSC");
  LOG_INFO(system_logger, "built-in System");
  LOG_INFO(user_logger, "application clock");
  CHECK_EQ(clock.reads.load(), 1);

  // Cloning preserves the clock object and its selected source.
  quill::Logger* clone = quill::Frontend::create_or_get_logger("clock_source_selection_clone", user_logger);
  CHECK_EQ(clone->get_user_clock_source(), std::addressof(clock));
  LOG_INFO(clone, "shared application clock");
  CHECK_EQ(clock.reads.load(), 2);

  quill::Backend::start();
  clone->flush_log();
  quill::Frontend::remove_logger_blocking(tsc_logger);
  quill::Frontend::remove_logger_blocking(system_logger);
  quill::Frontend::remove_logger_blocking(user_logger);
  quill::Frontend::remove_logger_blocking(clone);
  quill::Backend::stop();
}
