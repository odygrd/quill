#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Atomic.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_atomic_logging")
{
  static constexpr char const* filename = "std_atomic_logging.log";
  static std::string const logger_name = "std_atomic_logger";

  Backend::start();

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

  std::atomic<bool> const boolean{true};
  std::atomic<double> const dbl{1.4};

  LOG_INFO(logger, "bool_atomic {}", boolean);
  LOG_INFO(logger, "double_atomic {}", dbl);
  LOG_INFO(logger, "temp_atomic {}", std::atomic<uint16_t>{65534});

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " bool_atomic true"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " double_atomic 1.4"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " temp_atomic 65534"));

  testing::remove_file(filename);
}
