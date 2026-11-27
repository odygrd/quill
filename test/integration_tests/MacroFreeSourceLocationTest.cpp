#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/sinks/FileSink.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

using namespace quill;

namespace
{
struct Incomplete;

/** A format string that converts to char const*, e.g. a string class of an application. Its
 * template argument cannot be instantiated, as Incomplete is never defined, and the call must not
 * need it */
template <typename Tag>
struct ConvertibleFormat
{
  operator char const*() const noexcept { return "Converted format {}"; }
};

QUILL_MAYBE_UNUSED size_t log_with_a_converted_format(Logger* logger)
{
  quill::info(logger, ConvertibleFormat<std::pair<Incomplete, int>>{}, 1);
  return __LINE__ - 1;
}

QUILL_MAYBE_UNUSED void log_from_a_large_line(Logger* logger);
} // namespace

/***/
TEST_CASE("macro_free_source_location")
{
#if defined(QUILL_DISABLE_FILE_INFO) || defined(QUILL_DISABLE_FUNCTION_NAME)
  // The expected output below has the file, line and function of each call
  return;
#else

  static constexpr char const* filename = "macro_free_source_location.log";
  static std::string const logger_name = "macro_free_source_location_logger";

  Backend::start();

  Logger* logger = Frontend::create_or_get_logger(
    logger_name,
    Frontend::create_or_get_sink<FileSink>(
      filename,
      []()
      {
        FileSinkConfig cfg;
        cfg.set_open_mode('w');
        return cfg;
      }(),
      FileEventNotifier{}),
    PatternFormatterOptions{"%(source_location) | %(short_source_location) | %(caller_function) | "
                            "LOG_%(log_level) | %(message)"});

  // A call site whose line does not fit the facts that a literal format string packs copies its
  // source location
  log_from_a_large_line(logger);

  // A format string that converts to char const* is copied
  size_t const converted_line = log_with_a_converted_format(logger);

  // An explicit source location, with explicit template arguments as before
  SourceLocation const location{"generated/request_handler.cpp", "handle", 4321};
  quill::info<Logger, int>(logger, "Literal with location {}", 2, location);
  char const* const runtime_format = "Runtime with location {}";
  quill::warning<Logger, int>(logger, runtime_format, 3, location);

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 4);

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{
      "MacroFreeSourceLocationTest.cpp:1200000 | MacroFreeSourceLocationTest.cpp:1200000 | "
      "log_from_a_large_line | LOG_INFO | Large line 7"}));

  std::string const converted_location =
    std::string{"MacroFreeSourceLocationTest.cpp:"} + std::to_string(converted_line);
  std::string const converted_message =
    " | log_with_a_converted_format | LOG_INFO | Converted format 1";
  REQUIRE(quill::testing::file_contains(
    file_contents, converted_location + " | " + converted_location + converted_message));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"generated/request_handler.cpp:4321 | request_handler.cpp:4321 | handle | "
                "LOG_INFO | Literal with location 2"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"generated/request_handler.cpp:4321 | request_handler.cpp:4321 | handle | "
                "LOG_WARNING | Runtime with location 3"}));

  testing::remove_file(filename);
#endif
}

// These change the line and the file of the code that follows, so they come last
namespace
{
void log_from_a_large_line(Logger* logger)
{
  // clang-format off
#line 1200000
  quill::info(logger, "Large line {}", 7);
  // clang-format on
}
} // namespace
