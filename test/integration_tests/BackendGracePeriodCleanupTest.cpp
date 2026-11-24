#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/UserClockSource.h"
#include "quill/core/Rdtsc.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

namespace
{
/** A TSC clock that is ahead of the wall clock, which the conversion of the ticks can also be */
class AheadTscClock : public UserClockSource
{
public:
  uint64_t now() const noexcept override { return detail::rdtsc() + (uint64_t{1} << 32u); }

  TimestampType timestamp_type() const noexcept override { return TimestampType::TscTicks; }
};
} // namespace

/***/
TEST_CASE("backend_grace_period_cleanup")
{
  static constexpr char const* filename = "backend_grace_period_cleanup.log";
  static constexpr size_t short_lived_threads = 20;

  // A thread that logs every millisecond keeps a record waiting for the grace period at all times,
  // so the queues are never all empty
  std::atomic<uint64_t> poll_count{0};

  BackendOptions backend_options;
  backend_options.log_timestamp_ordering_grace_period = std::chrono::seconds{1};
  backend_options.sleep_duration = std::chrono::milliseconds{1};
  backend_options.backend_worker_on_poll_begin = [&poll_count]()
  { poll_count.fetch_add(1, std::memory_order_relaxed); };
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* steady_logger = Frontend::create_or_get_logger("grace_period_cleanup_steady", file_sink);
  Logger* session_logger = Frontend::create_or_get_logger("grace_period_cleanup_session", file_sink);

  std::atomic<bool> stop_producer{false};
  std::atomic<size_t> produced{0};

  std::thread producer{[steady_logger, &stop_producer, &produced]()
                       {
                         while (!stop_producer.load())
                         {
                           LOG_INFO(steady_logger, "steady");
                           produced.fetch_add(1);
                           std::this_thread::sleep_for(std::chrono::milliseconds{1});
                         }
                       }};

  LOG_INFO(session_logger, "session");

  while (produced.load() == 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  auto const count_thread_contexts = []()
  {
    size_t count{0};
    detail::ThreadContextManager::instance().for_each_thread_context(
      [&count](detail::ThreadContext const*) { ++count; });
    return count;
  };

  size_t const steady_thread_contexts = count_thread_contexts();

  // The backend sleeps while the records wait, instead of polling the queues without a pause
  uint64_t const polls_begin = poll_count.load();
  auto const window_begin = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(std::chrono::milliseconds{300});
  uint64_t const polls = poll_count.load() - polls_begin;
  auto const window_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - window_begin)
      .count();

  CHECK_LT(polls, static_cast<uint64_t>(window_ms) * 100u);

  // The contexts of threads that have exited are released while the other thread keeps logging
  for (size_t i = 0; i < short_lived_threads; ++i)
  {
    std::thread short_lived{[steady_logger]() { LOG_INFO(steady_logger, "short lived"); }};
    short_lived.join();
  }

  auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds{20};

  while ((count_thread_contexts() != steady_thread_contexts) && (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  CHECK_EQ(count_thread_contexts(), steady_thread_contexts);

  // A logger is removed while the other thread keeps logging
  std::atomic<bool> removed{false};

  std::thread remover{[session_logger, &removed]()
                      {
                        Frontend::remove_logger_blocking(session_logger);
                        removed.store(true);
                      }};

  while (!removed.load() && (std::chrono::steady_clock::now() < deadline))
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
  }

  bool const removed_while_logging = removed.load();

  stop_producer.store(true);
  producer.join();
  remover.join();

  CHECK(removed_while_logging);

  Backend::stop();

  // The backend does not wait for the grace period when it stops, also for a record with a
  // timestamp that is ahead of the wall clock. The grace period is now much longer than the
  // stop may take on a busy machine
  backend_options.log_timestamp_ordering_grace_period = std::chrono::seconds{10};
  Backend::start(backend_options);

  AheadTscClock ahead_clock;
  Logger* ahead_logger = Frontend::create_or_get_logger(
    "grace_period_cleanup_ahead", file_sink, PatternFormatterOptions{}, ClockSource::custom(ahead_clock));
  LOG_INFO(steady_logger, "within the grace period");
  LOG_INFO(ahead_logger, "ahead");

  auto const stop_begin = std::chrono::steady_clock::now();
  Backend::stop();
  auto const stop_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - stop_begin)
      .count();

  CHECK_LT(stop_ms, 5000);

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  CHECK_EQ(file_contents.size(), produced.load() + short_lived_threads + 3u);

  testing::remove_file(filename);
}
