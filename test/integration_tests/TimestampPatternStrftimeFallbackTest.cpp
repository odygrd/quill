#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <utility>
#include <vector>

using namespace quill;

/***/
TEST_CASE("timestamp_pattern_strftime_fallback")
{
  static constexpr char const* filename = "timestamp_pattern_strftime_fallback.log";
  static std::string const logger_name = "timestamp_pattern_strftime_fallback_logger";

  // %X embeds the time of day, which the cached timestamp formatter cannot update. The backend
  // formats it with strftime() instead and reports this once, when it creates the formatter.
  BackendOptions backend_options;
  std::vector<std::string> notifications;
  backend_options.error_notifier = [&notifications](std::string const& error_message)
  { notifications.push_back(error_message); };
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

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink), PatternFormatterOptions{"%(time) %(message)", "%X", Timezone::GmtTime});

  LOG_INFO(logger, "first");
  LOG_INFO(logger, "second");

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  REQUIRE_EQ(notifications.size(), 1u);
  REQUIRE_NE(notifications.front().find("`%X`"), std::string::npos);

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 2u);
  REQUIRE(testing::file_contains(file_contents, " first"));
  REQUIRE(testing::file_contains(file_contents, " second"));

  testing::remove_file(filename);
}
