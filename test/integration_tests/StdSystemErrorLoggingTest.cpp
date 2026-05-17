#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/SystemError.h"

#include <cstdio>
#include <string>
#include <system_error>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_system_error_logging")
{
  static constexpr char const* filename = "std_system_error_logging.log";
  static std::string const logger_name = "std_system_error_logger";

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

  std::error_code const error_code_value = std::make_error_code(std::errc::permission_denied);
  std::error_code const default_error_code{};

  LOG_INFO(logger, "error_code {}", error_code_value);
  LOG_INFO(logger, "error_code_message {:s}", error_code_value);
  LOG_INFO(logger, "error_code_debug {:?}", error_code_value);
  LOG_INFO(logger, "default_error_code {}", default_error_code);
  LOG_INFO(logger, "temp_error_code {}", std::make_error_code(std::errc::io_error));

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  std::string const error_code_display =
    std::string{error_code_value.category().name()} + ":" + std::to_string(error_code_value.value());
  std::error_code const temp_error_code = std::make_error_code(std::errc::io_error);
  std::string const temp_error_code_display =
    std::string{temp_error_code.category().name()} + ":" + std::to_string(temp_error_code.value());
  std::string const default_error_code_display = std::string{default_error_code.category().name()} +
    ":" + std::to_string(default_error_code.value());

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " error_code " + error_code_display));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " error_code_message " + error_code_value.message()));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " error_code_debug " + fmtquill::format("{:?}", error_code_display)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " default_error_code " + default_error_code_display));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " temp_error_code " + temp_error_code_display));

  testing::remove_file(filename);
}
