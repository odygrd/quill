#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/DeferredFormatCodec.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

struct SlowCountingSink final : public quill::Sink
{
  void write_log(LogRecord const& /* record */) override
  {
    writes.fetch_add(1, std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::microseconds{50});
  }

  void flush_sink(SinkFlushReason) noexcept override {}

  std::atomic<size_t> writes{0};
};

namespace
{
constexpr size_t number_of_request_threads = 20;

std::atomic<size_t> timeout_poll_formats{0};

/** Logs the next request, or sets it as an MDC value, on the thread that calls it */
std::function<void()> log_next_request;

/**
 * An argument that is slow to format. While the backend formats one, a new thread logs the next
 * request, up to number_of_request_threads, so that each request arrives during the poll that
 * formats the one before it
 */
struct SlowToFormat
{
};
} // namespace

template <>
struct fmtquill::formatter<SlowToFormat>
{
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  auto format(SlowToFormat const& /* slow_to_format */, format_context& ctx) const -> decltype(ctx.out())
  {
    size_t const previous_formats = timeout_poll_formats.fetch_add(1);

    if (previous_formats < number_of_request_threads)
    {
      std::thread request_thread{log_next_request};
      request_thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{10});
    return ctx.out();
  }
};

template <>
struct quill::Codec<SlowToFormat> : quill::DeferredFormatCodec<SlowToFormat>
{
};

/**
 * poll(timeout) returns after the poll that runs out of time, and the next call goes on with the
 * records that are left. A batch writes a limited number of events, and reads a limited number of
 * queues: threads that keep starting to log, each with a record that is slow to format, kept a
 * batch reading their queues for as long as they did, before it wrote its first event. So did
 * threads that only set an MDC value that is slow to format, as a read that caches no event was
 * not counted.
 */
TEST_CASE("manual_backend_worker_timeout_poll_drains_incrementally")
{
  static std::string const sink_name = "manual_backend_worker_timeout_poll_sink";
  static std::string const logger_name = "manual_backend_worker_timeout_poll_logger";
  static constexpr size_t number_of_messages = 512;

  ManualBackendWorker* worker = Backend::acquire_manual_backend_worker();
  BackendOptions backend_options;
  backend_options.transit_event_buffer_initial_capacity = 2;
  backend_options.transit_events_soft_limit = 1;
  backend_options.transit_events_hard_limit = 2;
  backend_options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  worker->init(backend_options);

  auto slow_sink = Frontend::create_or_get_sink<SlowCountingSink>(sink_name);
  Logger* logger = Frontend::create_or_get_logger(logger_name, slow_sink);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "timeout poll message {}", i);
  }

  worker->poll(std::chrono::milliseconds{1});

  auto* sink_ptr = static_cast<SlowCountingSink*>(slow_sink.get());
  size_t const partial_writes = sink_ptr->writes.load(std::memory_order_relaxed);

  worker->poll();

  size_t const drained_writes = sink_ptr->writes.load(std::memory_order_relaxed);

  // A record that is slow to format. While the backend formats it, and each request after it, a
  // new thread logs the next request
  auto requests_sink = Frontend::create_or_get_sink<SlowCountingSink>(sink_name + "_requests");
  Logger* requests_logger = Frontend::create_or_get_logger(logger_name + "_requests", requests_sink);

  log_next_request = [requests_logger]() { LOG_INFO(requests_logger, "request {}", SlowToFormat{}); };
  LOG_INFO(requests_logger, "first {}", SlowToFormat{});

  worker->poll(std::chrono::milliseconds{1});

  // The poll formats the first record and the record of the first thread, and is out of time.
  // Before, it went on with the record of every thread that started logging meanwhile
  size_t const formats_in_timeout_poll = timeout_poll_formats.load();

  worker->poll();

  auto* requests_sink_ptr = static_cast<SlowCountingSink*>(requests_sink.get());
  size_t const drained_request_formats = timeout_poll_formats.load();
  size_t const drained_request_writes = requests_sink_ptr->writes.load(std::memory_order_relaxed);

  // The same with threads that only set an MDC value that is slow to format. The read of their
  // queue caches no event
  timeout_poll_formats.store(0);

  auto mdc_sink = Frontend::create_or_get_sink<SlowCountingSink>(sink_name + "_mdc");
  Logger* mdc_logger = Frontend::create_or_get_logger(logger_name + "_mdc", mdc_sink);

  log_next_request = [mdc_logger]() { mdc_logger->set_mdc("request", SlowToFormat{}); };
  LOG_INFO(mdc_logger, "first {}", SlowToFormat{});

  worker->poll(std::chrono::milliseconds{1});

  // The poll formats the first record and the MDC value of the first thread, and is out of time
  size_t const formats_in_mdc_timeout_poll = timeout_poll_formats.load();

  worker->poll();
  worker->shutdown();

  Frontend::remove_logger(logger);
  Frontend::remove_logger(requests_logger);
  Frontend::remove_logger(mdc_logger);

  // The checks come after the shutdown, so that a failed one leaves no records for the backend to
  // read when the process exits
  REQUIRE_GT(partial_writes, 0);
  REQUIRE_LT(partial_writes, number_of_messages);
  REQUIRE_EQ(drained_writes, number_of_messages);

  REQUIRE_GE(formats_in_timeout_poll, 1);
  REQUIRE_LT(formats_in_timeout_poll, 5);
  REQUIRE_EQ(drained_request_formats, number_of_request_threads + 1);
  REQUIRE_EQ(drained_request_writes, number_of_request_threads + 1);

  auto* mdc_sink_ptr = static_cast<SlowCountingSink*>(mdc_sink.get());
  REQUIRE_GE(formats_in_mdc_timeout_poll, 1);
  REQUIRE_LT(formats_in_mdc_timeout_poll, 5);
  REQUIRE_EQ(timeout_poll_formats.load(), number_of_request_threads + 1);
  REQUIRE_EQ(mdc_sink_ptr->writes.load(std::memory_order_relaxed), 1);
}
