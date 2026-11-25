#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/Sink.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

namespace
{
constexpr std::chrono::milliseconds grace_period{250};

/** A clock of the epoch, such as the time of a simulation, that runs an hour ahead of system time */
class BatchGracePeriodAheadClock : public UserClockSource
{
public:
  uint64_t now() const noexcept override
  {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                   (std::chrono::system_clock::now() + std::chrono::hours{1}).time_since_epoch())
                                   .count());
  }
};

/**
 * Keeps the records in the order the backend writes them, and calls on_next_write once, when it
 * writes the next record
 */
class BatchGracePeriodSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override
  {
    records.emplace_back(record.log_statement);

    if (on_next_write)
    {
      std::function<void()> const callback{std::move(on_next_write)};
      on_next_write = nullptr;
      callback();
    }
  }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
  std::function<void()> on_next_write;
};

/** Keeps the records in the order the backend writes them */
class BatchGracePeriodMdcSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override { records.emplace_back(record.log_statement); }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
};
} // namespace

/**
 * A record that waits for log_timestamp_ordering_grace_period does not end a batch. Before, a
 * thread that started logging during a backlog ended each batch for as long as its records
 * waited, so that the backlog was not written.
 *
 * The batch writes the records up to the cutoff of the read that left the record waiting. A
 * thread that starts logging during the batch has newer records, which a later read of the batch
 * caches once they are older than the grace period, and the batch does not write them before the
 * record that waits.
 *
 * With the system clock alone a batch never gets to such a record: the events cached when it
 * starts, which it may write, are all older. A record of a clock of the epoch that runs ahead is
 * cached as well, as such records do not wait for the grace period, so the batch could write the
 * newer record of the system clock before it ends.
 *
 * An MDC event of a logger with the system clock does not wait for the grace period, as it is not
 * written, so the record behind it in the queue is read.
 */
TEST_CASE("backend_transit_events_batch_grace_period")
{
  static std::string const logger_name = "backend_transit_events_batch_grace_period_logger";

  // A batch for the backlog, and a single event for a poll with fewer cached events
  BackendOptions backend_options;
  backend_options.transit_events_soft_limit = 3;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = grace_period;

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink = Frontend::create_or_get_sink<BatchGracePeriodSink>(
    "backend_transit_events_batch_grace_period_sink");

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, sink, PatternFormatterOptions{"%(message)"}, ClockSource::system());

  BatchGracePeriodAheadClock ahead_clock;
  Logger* ahead_logger = Frontend::create_or_get_logger(
    logger_name + "_ahead", sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(ahead_clock));

  auto mdc_sink = Frontend::create_or_get_sink<BatchGracePeriodMdcSink>(
    "backend_transit_events_batch_grace_period_mdc_sink");

  Logger* mdc_ahead_logger = Frontend::create_or_get_logger(logger_name + "_mdc_ahead", mdc_sink,
                                                            PatternFormatterOptions{"%(message)"},
                                                            ClockSource::custom(ahead_clock));

  // An MDC event of the system clock within the grace period, and a record of the ahead clock
  // behind it, which a single event writes
  std::thread mdc_thread{[logger, mdc_ahead_logger]()
                         {
                           logger->set_mdc("request_id", 7);
                           LOG_INFO(mdc_ahead_logger, "record after the mdc");
                         }};
  mdc_thread.join();

  manual_backend_worker->poll_one();
  std::vector<std::string> const mdc_records_after_poll = mdc_sink->records;

  // A backlog that is older than the grace period when the queues are read
  std::thread backlog_thread{[logger]()
                             {
                               for (uint32_t i = 1; i <= 4; ++i)
                               {
                                 LOG_INFO(logger, "backlog {}", i);
                               }
                             }};
  backlog_thread.join();

  std::this_thread::sleep_for(2 * grace_period);

  std::thread ahead_thread{[ahead_logger]() { LOG_INFO(ahead_logger, "ahead record"); }};
  ahead_thread.join();

  // Waits for the grace period when the batch reads it
  std::thread waiting_thread{[logger]() { LOG_INFO(logger, "waiting record"); }};
  waiting_thread.join();

  // A new thread logs while the backend writes the first record of the batch, and the backend
  // stays there until that record is older than the grace period, so a later read of the batch
  // caches the newer record
  sink->on_next_write = [logger]()
  {
    std::thread newer_thread{[logger]() { LOG_INFO(logger, "newer record"); }};
    newer_thread.join();

    std::this_thread::sleep_for(3 * grace_period);
  };

  // The batch writes the backlog, and neither the newer record nor the ahead one while the
  // waiting record waits
  manual_backend_worker->poll_one();

  // Disarmed, in case the batch ended before it wrote a record
  sink->on_next_write = nullptr;
  std::vector<std::string> const records_after_batch = sink->records;

  // The shutdown reads the waiting record without the grace period, and writes it before the newer
  // one. It comes before the checks, as the backend reads the ahead clock until it stops
  Frontend::remove_logger(logger);
  Frontend::remove_logger(ahead_logger);
  Frontend::remove_logger(mdc_ahead_logger);
  manual_backend_worker->shutdown();

  REQUIRE_EQ(mdc_records_after_poll.size(), 1);
  REQUIRE_EQ(mdc_records_after_poll[0], std::string{"record after the mdc\n"});

  std::vector<std::string> expected_records;
  for (uint32_t i = 1; i <= 4; ++i)
  {
    expected_records.push_back("backlog " + std::to_string(i) + "\n");
  }

  REQUIRE_EQ(records_after_batch.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(records_after_batch[i], expected_records[i]);
  }

  expected_records.emplace_back("waiting record\n");
  expected_records.emplace_back("newer record\n");
  expected_records.emplace_back("ahead record\n");

  REQUIRE_EQ(sink->records.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(sink->records[i], expected_records[i]);
  }
}
