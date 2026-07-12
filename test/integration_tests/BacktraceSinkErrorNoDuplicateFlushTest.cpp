#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/**
 * A sink that throws once, when it first sees the "BT-1" backtrace record, simulating a sink
 * I/O failure in the middle of a backtrace flush
 */
class ThrowOnceFileSink : public FileSink
{
public:
  using FileSink::FileSink;

  void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                 std::string_view thread_id, std::string_view thread_name,
                 std::string const& process_id, std::string_view logger_name, LogLevel log_level,
                 std::string_view log_level_description, std::string_view log_level_short_code,
                 std::vector<std::pair<std::string, std::string>> const* named_args,
                 std::string_view log_message, std::string_view log_statement) override
  {
    if (!_thrown && (log_statement.find("BT-1") != std::string_view::npos))
    {
      _thrown = true;
      QUILL_THROW(QuillError{"simulated sink failure"});
    }

    FileSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                        logger_name, log_level, log_level_description, log_level_short_code,
                        named_args, log_message, log_statement);
  }

private:
  bool _thrown{false};
};

/***/
TEST_CASE("backtrace_sink_error_no_duplicate_flush")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // Regression: when a sink threw in the middle of a backtrace flush, the already-written
  // backtrace records stayed stored and were written again by the next flush, duplicating them.
  // Sink failures are now caught per-sink during dispatch, so the flush completes and the
  // storage is cleared; either way a mid-flush sink failure must never replay written records
  static constexpr char const* filename = "backtrace_sink_error_no_duplicate_flush.log";
  static std::string const logger_name = "backtrace_no_dup_logger";

  BackendOptions backend_options;
  backend_options.error_notifier = [](std::string const&) {}; // suppress the simulated failure
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<ThrowOnceFileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  logger->init_backtrace(4, LogLevel::Error);

  LOG_BACKTRACE(logger, "BT-{}", 0);
  LOG_BACKTRACE(logger, "BT-{}", 1);

  // Triggers the first backtrace flush: BT-0 is written, then the sink throws on BT-1
  LOG_ERROR(logger, "trigger one");

  // A second flush must not write BT-0 again
  LOG_BACKTRACE(logger, "BT-{}", 2);
  LOG_ERROR(logger, "trigger two");

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);

  auto const count_lines_containing = [&file_contents](std::string const& what)
  {
    size_t count{0};
    for (std::string const& line : file_contents)
    {
      if (line.find(what) != std::string::npos)
      {
        ++count;
      }
    }
    return count;
  };

  // BT-0 was written by the first flush and must not be written again by the second
  REQUIRE_EQ(count_lines_containing("BT-0"), 1);

  // BT-1 was never written (the sink threw in its place) and is dropped, not replayed
  REQUIRE_EQ(count_lines_containing("BT-1"), 0);

  // BT-2 was stored after the failure and is written by the second flush
  REQUIRE_EQ(count_lines_containing("BT-2"), 1);
  REQUIRE_EQ(count_lines_containing("trigger two"), 1);

  testing::remove_file(filename);
#endif
}
