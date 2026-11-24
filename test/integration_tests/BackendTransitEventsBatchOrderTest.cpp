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
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

namespace
{
class BatchOrderClock : public UserClockSource
{
public:
  uint64_t now() const noexcept override { return _timestamp.load(std::memory_order_relaxed); }

  void set_timestamp(uint64_t timestamp) { _timestamp.store(timestamp, std::memory_order_relaxed); }

private:
  std::atomic<uint64_t> _timestamp{0};
};

/**
 * Keeps the records in the order the backend writes them. While the backend writes the first one
 * it waits for another thread to log, so that a record arrives in the middle of a batch.
 */
class BatchOrderSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override
  {
    records.emplace_back(record.log_statement);

    if (records.size() == 1)
    {
      first_record_written.store(true);

      while (!record_logged_during_batch.load())
      {
        std::this_thread::yield();
      }
    }
  }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
  std::atomic<bool> first_record_written{false};
  std::atomic<bool> record_logged_during_batch{false};
};
} // namespace

/**
 * The backend caches up to transit_events_hard_limit records of each thread and processes the
 * cached records in a batch. It reads the queues again before it goes on with the batch when a
 * queue has records that can be older than the cached ones: the queue of a thread whose cached
 * records ran out, and the queue of a thread that logged while the batch was processed.
 */
TEST_CASE("backend_transit_events_batch_order")
{
  static std::string const logger_name = "backend_transit_events_batch_order_logger";

  BackendOptions backend_options;
  backend_options.transit_event_buffer_initial_capacity = 2;
  backend_options.transit_events_soft_limit = 1;
  backend_options.transit_events_hard_limit = 2;
  backend_options.sleep_duration = std::chrono::nanoseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink =
    Frontend::create_or_get_sink<BatchOrderSink>("backend_transit_events_batch_order_sink");

  BatchOrderClock clock;
  Logger* logger = Frontend::create_or_get_logger(
    logger_name, sink, PatternFormatterOptions{"%(message)"}, ClockSource::custom(clock));

  // Each thread logs to a queue of its own
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

  std::thread second_thread{[&log_record]()
                            {
                              for (uint64_t const timestamp : {50u, 60u, 70u, 80u})
                              {
                                log_record(timestamp);
                              }
                            }};
  second_thread.join();

  // A thread that has no record when the others are read. It logs while the backend writes the
  // first record of the batch, a record that is older than the others that are cached
  std::thread third_thread{[&log_record, &sink]()
                           {
                             while (!sink->first_record_written.load())
                             {
                               std::this_thread::yield();
                             }

                             log_record(15);
                             sink->record_logged_during_batch.store(true);
                           }};

  // Records 10, 20, 50 and 60 are cached. After 10, record 15 is in the queue of the third thread
  // and has to be read before 20. After 20 the first thread has no cached record left, and 30
  // and 40 are in its queue, so 50 and 60 are not written yet
  manual_backend_worker->poll_one();
  third_thread.join();

  manual_backend_worker->poll();

  std::vector<std::string> expected_records;
  for (uint64_t const timestamp : {10u, 15u, 20u, 30u, 40u, 50u, 60u, 70u, 80u})
  {
    expected_records.push_back("record " + std::to_string(timestamp) + "\n");
  }

  REQUIRE_EQ(sink->records.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(sink->records[i], expected_records[i]);
  }

  Frontend::remove_logger(logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();
}
