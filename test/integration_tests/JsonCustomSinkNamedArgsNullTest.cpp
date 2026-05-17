#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/JsonSink.h"

#include "quill/bundled/fmt/format.h"

#include <atomic>
#include <string>
#include <vector>

using namespace quill;

namespace
{
// Custom JSON sink that records, per log call, whether the named_args pointer it received was
// nullptr or non-nullptr. This mirrors the pattern users follow when their custom
// generate_json_message() needs to branch between "has named args" and "no named args".
class RecordingJsonSink : public JsonFileSink
{
public:
  using JsonFileSink::JsonFileSink;

  void generate_json_message(MacroMetadata const* /* log_metadata */, uint64_t /* log_timestamp */,
                             std::string_view /* thread_id */, std::string_view /* thread_name */,
                             std::string const& /* process_id */, std::string_view /* logger_name */,
                             LogLevel /* log_level */, std::string_view /* log_level_description */,
                             std::string_view /* log_level_short_code */,
                             std::vector<std::pair<std::string, std::string>> const* named_args,
                             std::string_view message, std::string_view /* log_statement */,
                             char const* /* message_format */) override
  {
    bool const has_args_marker = (message.find("has_named_args=true") != std::string_view::npos);
    bool const named_args_non_null = (named_args != nullptr);

    if (has_args_marker && !named_args_non_null)
    {
      missing_named_args_when_expected.fetch_add(1, std::memory_order_relaxed);
    }
    else if (!has_args_marker && named_args_non_null)
    {
      stale_named_args_when_none.fetch_add(1, std::memory_order_relaxed);
    }

    _json_message.append(fmtquill::format(R"({{"message":"{}","has_args":"{}"}})", message,
                                          named_args_non_null ? "true" : "false"));
  }

  static std::atomic<size_t> missing_named_args_when_expected;
  static std::atomic<size_t> stale_named_args_when_none;
};

std::atomic<size_t> RecordingJsonSink::missing_named_args_when_expected{0};
std::atomic<size_t> RecordingJsonSink::stale_named_args_when_none{0};
} // namespace

/**
 * Regression test for: a custom JsonSink that branches on `named_args == nullptr` to detect
 * messages without named arguments would see a non-null pointer to a stale empty vector when
 * the underlying TransitEvent slot had been used by a previous log message with named args.
 *
 * The interleaved pattern (named -> non-named -> warning) and the high iteration count are what
 * the bug report used to reliably reproduce the issue.
 */
TEST_CASE("json_custom_sink_named_args_null_when_no_named_args")
{
  static constexpr char const* filename = "json_custom_sink_named_args_null.json";

  RecordingJsonSink::missing_named_args_when_expected.store(0, std::memory_order_relaxed);
  RecordingJsonSink::stale_named_args_when_none.store(0, std::memory_order_relaxed);

  Backend::start(BackendOptions{});

  auto sink = Frontend::create_or_get_sink<RecordingJsonSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger =
    Frontend::create_or_get_logger("json_custom_sink_named_args_null_logger", std::move(sink));

#if defined(QUILL_ENABLE_EXTENSIVE_TESTS)
  static constexpr size_t iterations = 100000;
#else
  static constexpr size_t iterations = 5000;
#endif

  for (size_t i = 0; i < iterations; ++i)
  {
    LOG_INFO(logger, "has_named_args=true {name}", "arg1");
    LOG_INFO(logger, "has_named_args=false {}", "arg1");
    LOG_WARNING(logger, "Unrelated warning");
  }

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  REQUIRE_EQ(RecordingJsonSink::missing_named_args_when_expected.load(std::memory_order_relaxed), 0u);
  REQUIRE_EQ(RecordingJsonSink::stale_named_args_when_none.load(std::memory_order_relaxed), 0u);

  testing::remove_file(filename);
}
