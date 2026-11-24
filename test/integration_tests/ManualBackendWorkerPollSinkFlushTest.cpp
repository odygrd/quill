#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

/**
 * Counts the backend calls that only happen on a poll which finds nothing left to process.
 */
struct PollSinkFlushCountingSink final : public quill::Sink
{
  void write_log(LogRecord const& /* record */) override
  {
    writes.fetch_add(1, std::memory_order_relaxed);
  }

  void flush_sink(SinkFlushReason flush_reason) noexcept override
  {
    flushes.fetch_add(1, std::memory_order_relaxed);

    if (flush_reason == SinkFlushReason::Explicit)
    {
      explicit_flushes.fetch_add(1, std::memory_order_relaxed);
    }
  }

  void run_periodic_tasks() override { periodic_tasks.fetch_add(1, std::memory_order_relaxed); }

  std::atomic<size_t> writes{0};
  std::atomic<size_t> flushes{0};
  std::atomic<size_t> explicit_flushes{0};
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
  size_t const periodic_tasks_before_timeout_poll = sink_ptr->periodic_tasks.load(std::memory_order_relaxed);
  manual_backend_worker->poll(std::chrono::microseconds{50});
  REQUIRE_GT(sink_ptr->flushes.load(std::memory_order_relaxed), flushes_before_timeout_poll);
  REQUIRE_GT(sink_ptr->periodic_tasks.load(std::memory_order_relaxed), periodic_tasks_before_timeout_poll);

  // The backend keeps the list of the sinks until a logger is created or removed. A sink that is
  // created after it has listed them is flushed like the others, and no longer once its logger
  // is removed
  auto late_sink = Frontend::create_or_get_sink<PollSinkFlushCountingSink>(sink_name + "_late");
  Logger* late_logger = Frontend::create_or_get_logger(logger_name + "_late", late_sink);
  manual_backend_worker->poll();
  REQUIRE_GT(late_sink->flushes.load(std::memory_order_relaxed), 0);

  Frontend::remove_logger(late_logger);
  manual_backend_worker->poll();
  size_t const late_sink_flushes = late_sink->flushes.load(std::memory_order_relaxed);
  manual_backend_worker->poll();
  REQUIRE_EQ(late_sink->flushes.load(std::memory_order_relaxed), late_sink_flushes);

  // flush_log() also flushes the sinks of a logger that waits to be removed, which can hold
  // records written before the removal
  auto other_sink = Frontend::create_or_get_sink<PollSinkFlushCountingSink>(sink_name + "_other");
  Logger* other_logger = Frontend::create_or_get_logger(logger_name + "_other", other_sink);

  std::atomic<detail::ThreadContext*> flusher_context{nullptr};

  std::thread flusher{[logger, other_logger, &flusher_context]()
                      {
                        Frontend::remove_logger(logger);
                        flusher_context.store(detail::get_local_thread_context<FrontendOptions>());
                        other_logger->flush_log();
                      }};

  // Poll only once the flush request is in the queue of the other thread. The logger stays
  // registered while a queue holds a record; a poll that finds the queues empty removes it, which
  // flushes its sinks in any case
  while (!flusher_context.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  while (flusher_context.load()->get_spsc_queue_union().unbounded_spsc_queue.empty())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  CHECK_EQ(sink_ptr->explicit_flushes.load(std::memory_order_relaxed), 0);

  // Stop at the poll that processes the flush request, before a later poll removes the logger
  while (other_sink->explicit_flushes.load(std::memory_order_relaxed) == 0)
  {
    manual_backend_worker->poll_one();
  }

  flusher.join();
  REQUIRE_EQ(sink_ptr->explicit_flushes.load(std::memory_order_relaxed), 1);

  Frontend::remove_logger(other_logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();
}
