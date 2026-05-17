#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/StopWatch.h"
#include "quill/sinks/FileSink.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/**
 * The StopWatch elapsed value is sampled on the frontend hot path at the log call site, not when
 * the backend later formats the message
 */
namespace
{
/** Extract the elapsed seconds printed inside "elapsed [<value>]" for the given marker. */
double parse_elapsed(std::vector<std::string> const& file_contents, std::string const& marker)
{
  for (auto const& line : file_contents)
  {
    size_t const marker_pos = line.find(marker);
    if (marker_pos == std::string::npos)
    {
      continue;
    }

    size_t const open_bracket = line.find('[', marker_pos);
    size_t const close_bracket = line.find(']', open_bracket);
    if ((open_bracket == std::string::npos) || (close_bracket == std::string::npos))
    {
      continue;
    }

    std::string const value = line.substr(open_bracket + 1, close_bracket - open_bracket - 1);
    return std::stod(value);
  }

  return -1.0;
}
} // namespace

/***/
TEST_CASE("stop_watch_logging")
{
  static constexpr char const* filename = "stop_watch_logging.log";
  static std::string const logger_name = "logger_stop_watch";

  // Start the logging backend thread
  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};
  Backend::start(bo);

  Frontend::preallocate();

  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  // 1) A stopwatch logged after sleeping ~200ms should report at least ~200ms.
  {
    StopWatchTsc swt;
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    LOG_INFO(logger, "after_sleep_tsc elapsed [{}]", swt);

    StopWatchChrono swc;
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    LOG_INFO(logger, "after_sleep_chrono elapsed [{}]", swc);
  }

  // 2) A freshly constructed stopwatch logged immediately must report a small value even though we
  // sleep for a long time before flushing. If elapsed were sampled at backend format time, the
  // value would be inflated by this sleep.
  {
    StopWatchTsc swt;
    LOG_INFO(logger, "immediate_tsc elapsed [{}]", swt);

    StopWatchChrono swc;
    LOG_INFO(logger, "immediate_chrono elapsed [{}]", swc);

    // Delay the backend from formatting the messages above for a noticeable period.
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }

  // Verify the format specifier path still works (6 significant digits via the double formatter).
  {
    StopWatchTsc swt;
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    LOG_INFO(logger, "format_spec elapsed [{:.6}]", swt);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  Backend::stop();
  REQUIRE_FALSE(Backend::is_running());

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  // 1) elapsed after sleeping ~200ms must be >= 0.2s (allow scheduling jitter via a small margin).
  double const after_sleep_tsc = parse_elapsed(file_contents, "after_sleep_tsc");
  double const after_sleep_chrono = parse_elapsed(file_contents, "after_sleep_chrono");
  REQUIRE_GE(after_sleep_tsc, 0.18);
  REQUIRE_GE(after_sleep_chrono, 0.18);

  // 2) elapsed for the immediately-logged stopwatch must be tiny, well below the 1s backend delay.
  double const immediate_tsc = parse_elapsed(file_contents, "immediate_tsc");
  double const immediate_chrono = parse_elapsed(file_contents, "immediate_chrono");
  REQUIRE_GE(immediate_tsc, 0.0);
  REQUIRE_GE(immediate_chrono, 0.0);
  REQUIRE_LT(immediate_tsc, 0.5);
  REQUIRE_LT(immediate_chrono, 0.5);

  // 3) format specifier line is present and parseable.
  double const format_spec = parse_elapsed(file_contents, "format_spec");
  REQUIRE_GE(format_spec, 0.04);

  testing::remove_file(filename);
}
