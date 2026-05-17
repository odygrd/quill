#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

using namespace quill;

/***/
TEST_CASE("error_notifier_disabled")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#endif

  static constexpr char const* filename = "error_notifier_disabled.log";

  BackendOptions backend_options;
  backend_options.error_notifier = {};
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

  Logger* logger = Frontend::create_or_get_logger("root_error_notifier_disabled", std::move(file_sink));

  LOG_INFO(logger, "{name}} text", "value");

  logger->flush_log();
  Frontend::remove_logger_blocking(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1u);
  REQUIRE(quill::testing::file_contains(file_contents, R"([Could not format log statement. message: "{name}} text")"));

  testing::remove_file(filename);
}
