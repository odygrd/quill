#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/core/Filesystem.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/FilesystemPath.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_filesystem_path_logging")
{
  static constexpr char const* filename = "std_filesystem_path_logging.log";
  static std::string const logger_name = "logger";

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

  // Set writing logging to a file
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

  {
    fs::path sp{"/usr/local/bin"};
    LOG_INFO(logger, "sp {} {} {}", sp, sp, sp);
    LOG_INFO(logger, "sp_2 {} {} {}", sp, sp, sp);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sp /usr/local/bin /usr/local/bin /usr/local/bin"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sp_2 /usr/local/bin /usr/local/bin /usr/local/bin"}));

  testing::remove_file(filename);
}