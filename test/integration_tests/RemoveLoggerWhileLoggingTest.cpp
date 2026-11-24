#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

namespace
{
/** Keeps the records in the order the backend writes them */
class RecordsSink final : public Sink
{
public:
  void write_log(LogRecord const& record) override { records.emplace_back(record.log_statement); }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::vector<std::string> records;
};
} // namespace

/**
 * A logger is removed when the records that were logged before its removal are processed. It
 * does not wait for a moment when every queue is empty, which threads that keep logging through
 * other loggers never leave.
 */
TEST_CASE("remove_logger_while_logging")
{
  static constexpr size_t max_polls = 100;

  BackendOptions backend_options;
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(backend_options);

  auto sink = Frontend::create_or_get_sink<RecordsSink>("remove_logger_while_logging_sink");

  Logger* removed_logger = Frontend::create_or_get_logger(
    "remove_logger_while_logging_removed", sink, PatternFormatterOptions{"%(message)"});
  Logger* busy_logger = Frontend::create_or_get_logger("remove_logger_while_logging_busy", sink,
                                                       PatternFormatterOptions{"%(message)"});

  // Records of the logger in two queues: of a thread that has exited, and of this thread
  std::thread exited_thread{[removed_logger]()
                            {
                              LOG_INFO(removed_logger, "removed 1");
                              LOG_INFO(removed_logger, "removed 2");
                            }};
  exited_thread.join();

  LOG_INFO(removed_logger, "removed 3");

  size_t const loggers_before_removal = Frontend::get_number_of_loggers();
  Frontend::remove_logger(removed_logger);

  // A record is logged before each poll, so the backend never finds the queue of this thread empty
  size_t polls{0};

  while ((Frontend::get_number_of_loggers() == loggers_before_removal) && (polls < max_polls))
  {
    LOG_INFO(busy_logger, "busy {}", polls);
    manual_backend_worker->poll_one();
    ++polls;
  }

  REQUIRE_LT(polls, max_polls);
  REQUIRE_EQ(Frontend::get_number_of_loggers(), loggers_before_removal - 1);

  // The records of the logger were written before it was removed
  std::vector<std::string> expected_records{"removed 1\n", "removed 2\n", "removed 3\n"};
  REQUIRE_GE(sink->records.size(), expected_records.size());

  manual_backend_worker->poll();

  for (size_t i = 0; i < polls; ++i)
  {
    expected_records.push_back("busy " + std::to_string(i) + "\n");
  }

  REQUIRE_EQ(sink->records.size(), expected_records.size());

  for (size_t i = 0; i < expected_records.size(); ++i)
  {
    REQUIRE_EQ(sink->records[i], expected_records[i]);
  }

  Frontend::remove_logger(busy_logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();
}
