#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("unicode_file_path_logging")
{
  // Cyrillic and CJK characters, which most Windows ANSI code pages cannot encode
#if defined(_WIN32)
  fs::path const directory{
    L"unicode_file_path_logging_\u041a\u0443\u0437\u044c\u043c\u0430_\u65e5\u8a8c"};
#else
  fs::path const directory{
    "unicode_file_path_logging_\xd0\x9a\xd1\x83\xd0\xb7\xd1\x8c\xd0\xbc\xd0\xb0_"
    "\xe6\x97\xa5\xe8\xaa\x8c"};
#endif
  fs::path const filename{directory / "unicode_file_path_logging.log"};
  static std::string const logger_name = "unicode_file_path_logging";

  Backend::start();

  FileSinkConfig config;
  config.set_open_mode('w');
  auto file_sink = Frontend::create_or_get_sink<FileSink>(filename, config);
  REQUIRE(fs::equivalent(file_sink->get_filename(), filename));

  // The same path returns the same sink
  REQUIRE_EQ(Frontend::create_or_get_sink<FileSink>(filename, config), file_sink);

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));
  LOG_INFO(logger, "Logged to a Unicode path");
  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 1);
  REQUIRE(testing::file_contains(file_contents, "Logged to a Unicode path"));

  testing::remove_file(filename);
  fs::remove(directory);
}
