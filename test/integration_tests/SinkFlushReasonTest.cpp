#include "doctest/doctest.h"

#include "misc/TestUtilities.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/Sink.h"
#include "quill/sinks/FileSink.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
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

class FinalFlushErrorSink final : public Sink
{
public:
  explicit FinalFlushErrorSink(std::string error) : _error(std::move(error)) {}

  void write_log(MacroMetadata const*, uint64_t, std::string_view, std::string_view,
                 std::string const&, std::string_view, LogLevel, std::string_view, std::string_view,
                 std::vector<std::pair<std::string, std::string>> const*, std::string_view, std::string_view) override
  {
  }

  void flush_sink() override {}

  void flush_sink(SinkFlushReason reason) override
  {
    // Bound failures so an accidental retry loop fails assertions instead of hanging CI.
    if (reason == SinkFlushReason::Final && ++final_flushes <= 4)
    {
      QUILL_THROW(QuillError{_error});
    }
  }

  size_t final_flushes{0};

private:
  std::string _error;
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
  REQUIRE_EQ(flush_reason_sink->final_flushes.load(), 1);
  REQUIRE_EQ(flush_reason_sink->legacy_flushes.load(), 0);

#if !defined(QUILL_NO_EXCEPTIONS)
  char const* diagnostic_filename = "sink_flush_reason_diagnostics.log";
  FileSinkConfig file_config;
  file_config.set_open_mode('w');
  auto diagnostic_sink = Frontend::create_sink<FileSink>(diagnostic_filename, file_config);
  auto* diagnostic_logger = Frontend::create_logger("sink_flush_reason_diagnostics", diagnostic_sink);
  uint32_t notifications{0};
  backend_options.error_notifier = [diagnostic_logger, &notifications](std::string const& error)
  {
    ++notifications;
    LOG_ERROR(diagnostic_logger, "{}", error);
  };

  auto first_sink = std::make_shared<FinalFlushErrorSink>("first final flush failed");
  auto second_sink = std::make_shared<FinalFlushErrorSink>("second final flush failed");
  auto* failing_logger = Frontend::create_logger(
    "sink_flush_reason_failures", std::vector<std::shared_ptr<Sink>>{first_sink, second_sink});
  Backend::start(backend_options);
  LOG_INFO(failing_logger, "buffered record");
  Backend::stop();

  CHECK_EQ(first_sink->final_flushes, 2);
  CHECK_EQ(second_sink->final_flushes, 2);
  CHECK_EQ(notifications, 2);
  // Keep the sink alive: this verifies the additional flush, not destructor I/O.
  auto const diagnostics = testing::file_contents(diagnostic_filename);
  CHECK_EQ(diagnostics.size(), 2);
  CHECK(testing::file_contains(diagnostics, "first final flush failed"));
  CHECK(testing::file_contains(diagnostics, "second final flush failed"));

  Backend::start();
  Frontend::remove_logger_blocking(failing_logger);
  Frontend::remove_logger_blocking(diagnostic_logger);
  Backend::stop();
  diagnostic_sink.reset();
  testing::remove_file(diagnostic_filename);
#endif
}
