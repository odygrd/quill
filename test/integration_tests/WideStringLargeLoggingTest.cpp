#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/WideString.h"

#include <string>
#include <vector>

using namespace quill;

/**
 * Regression test for the WideString codec under large inputs: the codec stores its length
 * as a uint32_t on the wire and was previously susceptible to truncate-then-overflow when
 * given a pathologically large string. The fix clamps the length safely. This test verifies
 * the round-trip works for a moderately-large wide string.
 *
 * Gated behind QUILL_ENABLE_EXTENSIVE_TESTS because it allocates a sizeable wstring and
 * writes a large line to disk; it is not needed on every CI run.
 */
TEST_CASE("wide_string_large_logging")
{
#if defined(_WIN32) && defined(QUILL_ENABLE_EXTENSIVE_TESTS)
  static constexpr char const* filename = "wide_string_large_logging.log";
  static std::string const logger_name = "wide_string_large_logging_logger";

  Backend::start();

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

  static constexpr size_t large_len = 64 * 1024;
  std::wstring const large_ws(large_len, L'A');

  LOG_INFO(logger, "large [{}]", large_ws);

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  std::string const expected =
    "LOG_INFO      " + logger_name + "       large [" + std::string(large_len, 'A') + "]";
  REQUIRE(quill::testing::file_contains(file_contents, expected));

  testing::remove_file(filename);
#endif
}
