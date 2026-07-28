#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using namespace quill;

class FlushReasonSink final : public Sink
{
public:
  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view, std::string_view) override
  {
  }

  void flush_sink() noexcept override { ++legacy_flushes; }

  void flush_sink(SinkFlushReason flush_reason) noexcept override
  {
    switch (flush_reason)
    {
    case SinkFlushReason::Periodic:
      ++periodic_flushes;
      break;
    case SinkFlushReason::Explicit:
      ++explicit_flushes;
      break;
    case SinkFlushReason::Final:
      ++final_flushes;
      break;
    }
  }

  std::atomic<uint32_t> legacy_flushes{0};
  std::atomic<uint32_t> periodic_flushes{0};
  std::atomic<uint32_t> explicit_flushes{0};
  std::atomic<uint32_t> final_flushes{0};
};

TEST_CASE("sink_flush_reason_is_passed_to_custom_sinks")
{
  static std::string const sink_name = "sink_flush_reason_sink";
  static std::string const logger_name = "sink_flush_reason_logger";

  BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::milliseconds{1};
  backend_options.sink_min_flush_interval = std::chrono::milliseconds{0};
  Backend::start(backend_options);

  auto sink = Frontend::create_or_get_sink<FlushReasonSink>(sink_name);
  Logger* logger = Frontend::create_or_get_logger(logger_name, sink);

  auto* flush_reason_sink = static_cast<FlushReasonSink*>(sink.get());

  auto wait_until = [](auto condition)
  {
    for (uint32_t i = 0; i < 1000; ++i)
    {
      if (condition())
      {
        return true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

    return false;
  };

  REQUIRE(
    wait_until([flush_reason_sink]() { return flush_reason_sink->periodic_flushes.load() != 0; }));

  LOG_INFO(logger, "flush reason test");
  logger->flush_log();

  REQUIRE(
    wait_until([flush_reason_sink]() { return flush_reason_sink->explicit_flushes.load() != 0; }));

  Backend::stop();

  REQUIRE_GT(flush_reason_sink->periodic_flushes.load(), 0);
  REQUIRE_GT(flush_reason_sink->explicit_flushes.load(), 0);
  REQUIRE_GT(flush_reason_sink->final_flushes.load(), 0);
  REQUIRE_EQ(flush_reason_sink->legacy_flushes.load(), 0);
}
