#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

/***/
TEST_CASE("mdc_logging")
{
  static constexpr char const* filename = "mdc_logging.log";
  static std::string const logger_name_a = "logger_a";
  static std::string const logger_name_b = "logger_b";

  BackendOptions backend_options;
  backend_options.mdc_format_pattern = " <{} = {} | >";

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

  Logger* logger_a = Frontend::create_or_get_logger(
    logger_name_a, std::move(file_sink), PatternFormatterOptions("[%(logger)] %(message)%(mdc)"));
  Logger* logger_b = Frontend::create_or_get_logger(logger_name_b, logger_a);

  logger_a->set_mdc("request_id", 10, "user", "alice");
  LOG_INFO(logger_a, "main before replace");
  LOG_INFO(logger_b, "main shared");

  logger_a->set_mdc("request_id", 11);
  LOG_INFO(logger_a, "main replaced");

  logger_b->erase_mdc("missing", "user");
  LOG_INFO(logger_b, "main erased");

  std::thread worker{[logger_a, logger_b]()
                     {
                       LOG_INFO(logger_a, "worker before mdc");

                       logger_b->set_mdc("worker_id", 7);
                       LOG_INFO(logger_a, "worker set");

                       logger_a->clear_mdc();
                       LOG_INFO(logger_b, "worker cleared");
                     }};

  worker.join();

  LOG_INFO(logger_a, "main still there");

  logger_a->clear_mdc();
  LOG_INFO(logger_b, "main cleared");

  logger_a->flush_log();
  Frontend::remove_logger(logger_a);
  Frontend::remove_logger(logger_b);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 9);

  REQUIRE(quill::testing::file_contains(
    file_contents, "[logger_a] main before replace <request_id = 10 | user = alice>"));
  REQUIRE(quill::testing::file_contains(file_contents,
                                        "[logger_b] main shared <request_id = 10 | user = alice>"));
  REQUIRE(quill::testing::file_contains(
    file_contents, "[logger_a] main replaced <request_id = 11 | user = alice>"));
  REQUIRE(quill::testing::file_contains(file_contents, "[logger_b] main erased <request_id = 11>"));
  REQUIRE(quill::testing::file_contains(file_contents, "[logger_a] worker before mdc"));
  REQUIRE(quill::testing::file_contains(file_contents, "[logger_a] worker set <worker_id = 7>"));
  REQUIRE(quill::testing::file_contains(file_contents, "[logger_b] worker cleared"));
  REQUIRE(
    quill::testing::file_contains(file_contents, "[logger_a] main still there <request_id = 11>"));
  REQUIRE(quill::testing::file_contains(file_contents, "[logger_b] main cleared"));

  testing::remove_file(filename);
}
