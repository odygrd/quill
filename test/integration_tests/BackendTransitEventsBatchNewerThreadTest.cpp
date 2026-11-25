#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/sinks/Sink.h"

#include <atomic>
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
class BatchNewerThreadClock : public UserClockSource
{
public:
  uint64_t now() const noexcept override { return _timestamp.load(std::memory_order_relaxed); }

  void set_timestamp(uint64_t timestamp) { _timestamp.store(timestamp, std::memory_order_relaxed); }

private:
  std::atomic<uint64_t> _timestamp{0};
};

/**
 * Keeps the records in the order the backend writes them, and calls on_next_write once, when it
 * writes the next record
 */
class BatchNewerThreadSink final : public Sink
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
} // namespace

/**
 * A thread that starts logging during a batch, with records newer than the cached ones, does not
 * end the batch: its queue is read and the batch goes on, up to the number of events that were
 * cached when it started. Before, every such thread ended it, so that a backlog was not written
 * while new threads kept starting to log, and the memory of their queues grew.
 */
TEST_CASE("backend_transit_events_batch_newer_thread")
{
  static std::string const logger_name = "backend_transit_events_batch_newer_thread_logger";

  BackendOptions backend_options;
  backend_options.transit_events_soft_limit = 1;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink = Frontend::create_or_get_sink<BatchNewerThreadSink>(
    "backend_transit_events_batch_newer_thread_sink");

  BatchNewerThreadClock clock;
  Logger* logger = Frontend::create_or_get_logger(
    logger_name, sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(clock));

  auto const log_record = [logger, &clock](uint64_t timestamp)
  {
    clock.set_timestamp(timestamp);
    LOG_INFO(logger, "record {}", timestamp);
  };

  std::thread first_thread{[&log_record]()
                           {
                             for (uint64_t const timestamp : {10u, 20u, 30u, 40u})
                             {
                               log_record(timestamp);
                             }
                           }};
  first_thread.join();

  // A thread that has no record when the queues are read. It logs a newer record while the
  // backend writes the first record of the batch
  sink->on_next_write = [&log_record]()
  {
    std::thread second_thread{[&log_record]() { log_record(100); }};
    second_thread.join();
  };

  // The batch writes the four cached records, and reads record 100 from the queue on the way
  manual_backend_worker->poll_one();

  // Disarmed, in case the batch ended before it wrote a record
  sink->on_next_write = nullptr;
  std::vector<std::string> const records_after_batch = sink->records;

  manual_backend_worker->poll();
  std::vector<std::string> const records_after_poll = sink->records;

  // The shutdown comes before the checks, as the backend reads the clock until it stops
  Frontend::remove_logger(logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();

  std::vector<std::string> expected_records;
  for (uint64_t const timestamp : {10u, 20u, 30u, 40u})
  {
    expected_records.push_back("record " + std::to_string(timestamp) + "\n");
  }

  REQUIRE_EQ(records_after_batch.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(records_after_batch[i], expected_records[i]);
  }

  expected_records.emplace_back("record 100\n");

  REQUIRE_EQ(records_after_poll.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(records_after_poll[i], expected_records[i]);
  }
}
