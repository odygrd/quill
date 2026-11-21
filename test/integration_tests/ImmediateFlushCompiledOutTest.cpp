#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

struct NoImmediateFlushFrontendOptions : quill::FrontendOptions
{
  static constexpr bool enable_immediate_flush = false;
};

using NoImmediateFlushFrontend = quill::FrontendImpl<NoImmediateFlushFrontendOptions>;

/**
 * With enable_immediate_flush = false, set_immediate_flush() must not make log calls wait for the
 * backend. The manual backend below is not polled while the producer logs, so a flush would block
 * the producer.
 */
TEST_CASE("immediate_flush_compiled_out")
{
  class CapturingSink : public quill::Sink
  {
  public:
    void write_log(quill::LogRecord const& record) override
    {
      messages.emplace_back(record.log_message);
    }
    void flush_sink(quill::SinkFlushReason) override {}
    std::vector<std::string> messages;
  };

  quill::BackendOptions options;
  options.log_timestamp_ordering_grace_period = std::chrono::microseconds{0};
  auto* worker = quill::Backend::acquire_manual_backend_worker();
  worker->init(options);

  auto sink = NoImmediateFlushFrontend::create_or_get_sink<CapturingSink>(
    "immediate_flush_compiled_out_sink");
  auto* logger =
    NoImmediateFlushFrontend::create_or_get_logger("immediate_flush_compiled_out_logger", sink);
  logger->set_immediate_flush(1);

  std::atomic<bool> logged{false};
  std::thread producer(
    [logger, &logged]()
    {
      LOG_INFO(logger, "logged without flushing");
      logged.store(true);
    });

  auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (!logged.load() && (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::yield();
  }
  CHECK(logged.load());

  // Polling also releases the producer if the log call did wait for a flush
  while (!logged.load())
  {
    worker->poll();
  }
  producer.join();
  worker->poll();

  REQUIRE_EQ(sink->messages.size(), 1u);
  CHECK_EQ(sink->messages[0], "logged without flushing");

  worker->shutdown();
  NoImmediateFlushFrontend::remove_logger(logger);
}
