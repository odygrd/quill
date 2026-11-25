#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

using namespace quill;

namespace
{
/** Counts the records, and takes a while to write each, so that the backend has a backlog */
class SlowCountingSink final : public Sink
{
public:
  void write_log(LogRecord const& /* record */) override
  {
    // Spin instead of sleeping: a sleep of a few microseconds lasts a whole timer tick on
    // Windows, up to about 15 ms, which makes the backlog take longer than the test waits
    auto const until = std::chrono::steady_clock::now() + std::chrono::microseconds{20};
    while (std::chrono::steady_clock::now() < until)
    {
    }

    writes.fetch_add(1);
  }

  void flush_sink(SinkFlushReason /* flush_reason */) override {}

  std::atomic<size_t> writes{0};
};
} // namespace

/**
 * Frontend::remove_logger_blocking() returns while another thread keeps logging. The backend
 * writes the records in batches, and a batch reads the queue of a thread again when it has
 * written the cached records of that thread. A batch must not go on for as long as the thread
 * logs, or the backend never gets to remove the logger.
 */
TEST_CASE("remove_logger_blocking_while_logging")
{
  BackendOptions backend_options;

  // Every poll that reads a record writes a batch
  backend_options.transit_events_soft_limit = 1;
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  Backend::start(backend_options);

  auto sink =
    Frontend::create_or_get_sink<SlowCountingSink>("remove_logger_blocking_while_logging_sink");

  Logger* busy_logger = Frontend::create_or_get_logger("remove_logger_blocking_while_logging_busy", sink);

  Logger* removed_logger =
    Frontend::create_or_get_logger("remove_logger_blocking_while_logging_removed", sink);

  std::atomic<bool> stop_logging{false};
  std::thread busy_thread{[busy_logger, &sink, &stop_logging]()
                          {
                            size_t logged{0};

                            while (!stop_logging.load())
                            {
                              // Keep records queued ahead of the backend, also while this
                              // thread is not scheduled for a while
                              if ((logged - sink->writes.load()) < 1000)
                              {
                                LOG_INFO(busy_logger, "busy {}", logged);
                                ++logged;
                              }
                              else
                              {
                                std::this_thread::yield();
                              }
                            }
                          }};

  // The backend is busy with the records of the busy thread
  while (sink->writes.load() < 64)
  {
    std::this_thread::yield();
  }

  std::atomic<bool> removed{false};
  std::thread removing_thread{[removed_logger, &removed]()
                              {
                                Frontend::remove_logger_blocking(removed_logger);
                                removed.store(true);
                              }};

  // Wait with a timeout, so that a regression fails instead of waiting for as long as the busy
  // thread logs
  auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
  while (!removed.load() && (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  bool const removed_while_logging = removed.load();

  stop_logging.store(true);
  busy_thread.join();
  removing_thread.join();

  REQUIRE(removed_while_logging);

  Frontend::remove_logger(busy_logger);
  Backend::stop();
}
