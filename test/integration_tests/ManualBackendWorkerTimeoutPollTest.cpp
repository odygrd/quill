#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

struct SlowCountingSink final : public quill::Sink
{
  void write_log(quill::MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, quill::LogLevel, std::string_view,
                 std::string_view, std::vector<std::pair<std::string, std::string>> const*,
                 std::string_view, std::string_view) override
  {
    writes.fetch_add(1, std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::microseconds{50});
  }

  void flush_sink() noexcept override {}

  std::atomic<size_t> writes{0};
};

TEST_CASE("manual_backend_worker_timeout_poll_drains_incrementally")
{
  static std::string const sink_name = "manual_backend_worker_timeout_poll_sink";
  static std::string const logger_name = "manual_backend_worker_timeout_poll_logger";
  static constexpr size_t number_of_messages = 512;

  std::atomic<bool> backend_ready{false};
  std::atomic<bool> stop_backend{false};
  std::atomic<bool> run_timeout_poll{false};
  std::atomic<bool> timeout_poll_done{false};

  std::thread backend_thread(
    [&]()
    {
      ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();

      BackendOptions backend_options;
      manual_backend_worker->init(backend_options);
      backend_ready.store(true, std::memory_order_release);

      while (!run_timeout_poll.load(std::memory_order_acquire))
      {
        std::this_thread::yield();
      }

      manual_backend_worker->poll(std::chrono::microseconds{0});
      timeout_poll_done.store(true, std::memory_order_release);

      while (!stop_backend.load(std::memory_order_acquire))
      {
        std::this_thread::yield();
      }

      manual_backend_worker->poll();
      manual_backend_worker->shutdown();
    });

  while (!backend_ready.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  auto slow_sink = Frontend::create_or_get_sink<SlowCountingSink>(sink_name);
  Logger* logger = Frontend::create_or_get_logger(logger_name, slow_sink);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_INFO(logger, "timeout poll message {}", i);
  }

  run_timeout_poll.store(true, std::memory_order_release);

  while (!timeout_poll_done.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  auto* sink_ptr = static_cast<SlowCountingSink*>(slow_sink.get());
  size_t const partial_writes = sink_ptr->writes.load(std::memory_order_relaxed);
  REQUIRE_GT(partial_writes, 0);
  REQUIRE_LT(partial_writes, number_of_messages);

  stop_backend.store(true, std::memory_order_release);
  backend_thread.join();
  Frontend::remove_logger(logger);
  REQUIRE_EQ(sink_ptr->writes.load(std::memory_order_relaxed), number_of_messages);
}
