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
#include <utility>
#include <vector>

using namespace quill;

namespace
{
class TemporaryErrorCategory final : public std::error_category
{
public:
  char const* name() const noexcept override { return "temporary_error_category"; }

  std::string message(int value) const override
  {
    return "temporary error message " + std::to_string(value);
  }
};

void log_error_code_with_temporary_category(Logger* logger)
{
  TemporaryErrorCategory category;
  std::error_code const error_code_value{42, category};

  LOG_INFO(logger, "custom_error_code {}", error_code_value);
  LOG_INFO(logger, "custom_error_code_message {:s}", error_code_value);
}
} // namespace

/***/
TEST_CASE("std_system_error_logging")
{
  static constexpr char const* filename = "std_system_error_logging.log";
  static std::string const logger_name = "std_system_error_logger";

  ManualBackendWorker* manual_backend_worker = Backend::acquire_manual_backend_worker();
  manual_backend_worker->init(BackendOptions{});

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
  log_error_code_with_temporary_category(logger);

  manual_backend_worker->poll();
  Frontend::remove_logger(logger);
  manual_backend_worker->poll();
  manual_backend_worker->shutdown();

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
  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " custom_error_code temporary_error_category:42"));
  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " custom_error_code_message temporary error message 42"));

  testing::remove_file(filename);
}
