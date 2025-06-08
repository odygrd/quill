#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

class FileFilterA : public Filter
{
public:
  FileFilterA() : Filter("FileFilterA") {}

  QUILL_NODISCARD bool filter(quill::MacroMetadata const* log_metadata, uint64_t /** log_timestamp **/,
                              std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                              std::string_view /** logger_name **/, quill::LogLevel /** log_level **/,
                              std::string_view /** log_message **/, std::string_view log_statement) noexcept override
  {
    if (log_metadata->log_level() < LogLevel::Warning)
    {
      // we expect to match the override format pattern here
      REQUIRE_EQ(log_statement, std::string{"Lorem ipsum dolor sit amet, consectetur adipiscing elit [file]\n"});
      return true;
    }
    return false;
  }
};

class FileFilterB : public Filter
{
public:
  FileFilterB() : Filter("FileFilterB") {}

  bool filter(quill::MacroMetadata const* log_metadata, uint64_t /** log_timestamp **/,
              std::string_view /** thread_id **/, std::string_view /** thread_name **/,
              std::string_view /** logger_name **/, quill::LogLevel /** log_level **/,
              std::string_view /** log_message **/, std::string_view log_statement) noexcept override
  {
    if (log_metadata->log_level() >= LogLevel::Warning)
    {
      // we expect to match the override format pattern here
      REQUIRE_EQ(log_statement, std::string{"Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [logger]\n"});
      return true;
    }
    return false;
  }
};

/***/
TEST_CASE("sink_filter_override_format")
{
  // Tests that when we override the format of a sink, the filter gets the overridden format in log_statement
  static constexpr char const* filename_a = "sink_filter_override_format_a.log";
  static constexpr char const* filename_b = "sink_filter_override_format_b.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto file_sink_a = Frontend::create_or_get_sink<FileSink>(
    filename_a,
    []()
    {
      // Override format
      PatternFormatterOptions formatter_options_file_sink_a;
      formatter_options_file_sink_a.format_pattern = "%(message) [file]";

      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      cfg.set_override_pattern_formatter_options(formatter_options_file_sink_a);
      return cfg;
    }(),
    FileEventNotifier{});

  // log to filename_a anything below warning
  std::unique_ptr<Filter> filter_a = std::make_unique<FileFilterA>();

  // Also test get_filter_name()
  REQUIRE_EQ(filter_a->get_filter_name(), std::string_view{"FileFilterA"});

  // Add the filter
  file_sink_a->add_filter(std::move(filter_a));

  // Try to add the same again (same name)
  std::unique_ptr<Filter> filter_a_2 = std::make_unique<FileFilterA>();
  REQUIRE_EQ(filter_a_2->get_filter_name(), std::string_view{"FileFilterA"});
  REQUIRE_THROWS_AS(file_sink_a->add_filter(std::move(filter_a_2)), QuillError);

  auto file_sink_b = Frontend::create_or_get_sink<FileSink>(
    filename_b,
    []()
    {
      // No format override for file_sink_b
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  // log to filename_b warning, error, critical
  file_sink_b->add_filter(std::make_unique<FileFilterB>());

  Logger* logger =
    Frontend::create_or_get_logger(logger_name, {std::move(file_sink_a), std::move(file_sink_b)},
                                   PatternFormatterOptions{"%(message) [logger]"});

  LOG_INFO(logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
  LOG_ERROR(logger, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

  // Let all log get flushed to the file
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents_a = quill::testing::file_contents(filename_a);
  REQUIRE_EQ(file_contents_a.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents_a, std::string{"Lorem ipsum dolor sit amet, consectetur adipiscing elit [file]"}));

  std::vector<std::string> const file_contents_b = quill::testing::file_contents(filename_b);
  REQUIRE_EQ(file_contents_b.size(), 1);
  REQUIRE(quill::testing::file_contains(
    file_contents_b, std::string{"Nulla tempus, libero at dignissim viverra, lectus libero finibus ante [logger]"}));

  testing::remove_file(filename_a);
  testing::remove_file(filename_b);
}