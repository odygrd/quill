#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

using namespace quill;

namespace
{
/** Notes when the first record is written */
class GracePeriodSleepSink : public Sink
{
public:
  void write_log(LogRecord const&) override { written.store(true); }

  void flush_sink(SinkFlushReason) override {}

  std::atomic<bool> written{false};
};
} // namespace

/***/
TEST_CASE("backend_grace_period_sleep")
{
  // A record that waits for a grace period shorter than sleep_duration made the backend poll
  // without a pause until the grace period ended. It sleeps through the grace period instead, and
  // not for the whole sleep_duration
  std::atomic<uint64_t> polls{0};

  BackendOptions backend_options;
  backend_options.log_timestamp_ordering_grace_period = std::chrono::milliseconds{50};
  backend_options.sleep_duration = std::chrono::seconds{10};
  backend_options.backend_worker_on_poll_begin = [&polls]() { polls.fetch_add(1); };
  Backend::start(backend_options);

  auto sink = Frontend::create_or_get_sink<GracePeriodSleepSink>("backend_grace_period_sleep_sink");
  Logger* logger = Frontend::create_or_get_logger("backend_grace_period_sleep", sink,
                                                  PatternFormatterOptions{}, ClockSource::system());

  uint64_t const polls_before_log = polls.load();

  // The backend is asleep for sleep_duration until it is notified
  LOG_INFO(logger, "Waits for the grace period");
  Backend::notify();

  auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
  while (!sink->written.load() && (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  uint64_t const polls_until_written = polls.load() - polls_before_log;

  REQUIRE(sink->written.load());

  // A few polls: the one that finds the record in its grace period and the one after the sleep.
  // Polling without a pause made hundreds of thousands in the 50 ms
  REQUIRE_LT(polls_until_written, 100);

  // Not remove_logger_blocking(), which would wait for the backend to wake up after sleep_duration
  Backend::stop();
}
