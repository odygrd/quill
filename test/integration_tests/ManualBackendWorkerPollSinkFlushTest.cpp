#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace quill;

/**
 * Counts the backend calls that only happen on a poll which finds nothing left to process.
 */
struct PollSinkFlushCountingSink final : public quill::Sink
{
  void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, quill::LogLevel, std::string_view,
                 std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                 std::string_view, std::string_view) override
  {
    writes.fetch_add(1, std::memory_order_relaxed);
  }

  void flush_sink() noexcept override { flushes.fetch_add(1, std::memory_order_relaxed); }

  void run_periodic_tasks() override { periodic_tasks.fetch_add(1, std::memory_order_relaxed); }

  std::atomic<size_t> writes{0};
  std::atomic<size_t> flushes{0};
  std::atomic<size_t> periodic_tasks{0};
};

TEST_CASE("manual_backend_worker_poll_flushes_sinks")
{
  static std::string const sink_name = "manual_backend_worker_poll_sink_flush_sink";
  static std::string const logger_name = "manual_backend_worker_poll_sink_flush_logger";
  static constexpr size_t number_of_messages = 8;

  // This thread owns the manual backend worker for the whole test, so init(), the polling calls
  // and shutdown() all run on it, as the default (non migrating) mode requires
  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();

  BackendOptions backend_options;

  // Flush on every idle poll instead of once every sink_min_flush_interval, and read every queue
  // entry as soon as it arrives, so the assertions below do not depend on timing
  backend_options.sink_min_flush_interval = std::chrono::milliseconds{0};
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};

  manual_backend_worker->init(backend_options);

  auto flush_counting_sink = Frontend::create_or_get_sink<PollSinkFlushCountingSink>(sink_name);
  auto* sink_ptr = static_cast<PollSinkFlushCountingSink*>(flush_counting_sink.get());
  Logger* logger = Frontend::create_or_get_logger(logger_name, flush_counting_sink);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "poll sink flush message {}", i);
  }

  manual_backend_worker->poll();
  REQUIRE_EQ(sink_ptr->writes.load(std::memory_order_relaxed), number_of_messages);

  // Draining the queues is not enough on its own: the sinks must also be flushed and their
  // periodic tasks run, otherwise the records stay in the sink's own buffer and periodic sink
  // work never happens while the application keeps polling
  REQUIRE_GT(sink_ptr->flushes.load(std::memory_order_relaxed), 0);
  REQUIRE_GT(sink_ptr->periodic_tasks.load(std::memory_order_relaxed), 0);

  // The same holds when polling with a timeout and the queues are already drained
  size_t const flushes_before_timeout_poll = sink_ptr->flushes.load(std::memory_order_relaxed);
  size_t const periodic_tasks_before_timeout_poll =
    sink_ptr->periodic_tasks.load(std::memory_order_relaxed);
  manual_backend_worker->poll(std::chrono::microseconds{50});
  REQUIRE_GT(sink_ptr->flushes.load(std::memory_order_relaxed), flushes_before_timeout_poll);
  REQUIRE_GT(sink_ptr->periodic_tasks.load(std::memory_order_relaxed),
               periodic_tasks_before_timeout_poll);

  Frontend::remove_logger(logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();
}
