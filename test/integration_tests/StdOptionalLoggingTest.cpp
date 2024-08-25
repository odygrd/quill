#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Optional.h"

#include <array>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_optional_logging")
{
  static constexpr char const* filename = "std_optional_logging.log";
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
    std::optional<int> ei{std::nullopt};
    LOG_INFO(logger, "ei [{}]", ei);

    std::optional<double> ed{std::nullopt};
    LOG_INFO(logger, "ed [{}]", ed);

    std::optional<char const*> ccp{"testing"};
    LOG_INFO(logger, "ccp [{}]", ccp);

    std::optional<std::string> sp{"sp_testing"};
    LOG_INFO(logger, "sp [{}]", sp);

    std::optional<std::string> esp{std::nullopt};
    LOG_INFO(logger, "esp [{}]", esp);

    std::optional<std::string_view> svp{"svp_testing"};
    LOG_INFO(logger, "svp [{}]", svp);

    std::optional<int> i{123321};
    LOG_INFO(logger, "i [{}]", i);

    std::optional<double> d{333.221};
    LOG_INFO(logger, "d [{}]", d);

    LOG_INFO(logger, "zzzz [{}] [{}] [{}] [{}] [{}]", d, "test", *svp, *sp, ccp);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ei [none]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ed [none]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       ccp [optional(\"testing\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       sp [optional(\"sp_testing\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       esp [none]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       svp [optional(\"svp_testing\")]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       i [optional(123321)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       d [optional(333.221)]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       zzzz [optional(333.221)] [test] [svp_testing] [sp_testing] [optional(\"testing\")]"}));

  testing::remove_file(filename);
}