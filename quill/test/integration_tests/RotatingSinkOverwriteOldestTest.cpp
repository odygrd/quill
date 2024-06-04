#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/RotatingFileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("rotating_sink_overwrite_oldest")
{
  // This test has 2 loggers that they logged to different rotating file handlers
  static char const* base_filename_a = "rotating_sink_overwrite_oldest_a.log";
  static constexpr char const* base_filename_a_1 = "rotating_sink_overwrite_oldest_a.1.log";
  static constexpr char const* base_filename_a_2 = "rotating_sink_overwrite_oldest_a.2.log";

  static char const* base_filename_b = "rotating_sink_overwrite_oldest_b.log";
  static constexpr char const* base_filename_b_1 = "rotating_sink_overwrite_oldest_b.1.log";

  static constexpr size_t max_file_size = 900;

  // Start the logging backend thread
  Backend::start();

  // Create a rotating file handler
  std::shared_ptr<quill::Sink> rotating_file_sink_a =
    Frontend::create_or_get_sink<RotatingFileSink>(base_filename_a,
                                                   []()
                                                   {
                                                     RotatingFileSinkConfig rfh_cfg;
                                                     rfh_cfg.set_rotation_max_file_size(max_file_size);
                                                     rfh_cfg.set_max_backup_files(2);
                                                     rfh_cfg.set_overwrite_rolled_files(true);
                                                     rfh_cfg.set_open_mode('w');
                                                     return rfh_cfg;
                                                   }());

  // Get the same instance back - we search it again (for testing only)
  quill::Logger* logger_a = Frontend::create_or_get_logger("logger_a", std::move(rotating_file_sink_a));

  // Another rotating logger to another file with max backup count 1 this time. Here we rotate only once
  std::shared_ptr<quill::Sink> rotating_file_sink_b =
    Frontend::create_or_get_sink<RotatingFileSink>(base_filename_b,
                                                   []()
                                                   {
                                                     RotatingFileSinkConfig rfh_cfg;
                                                     rfh_cfg.set_rotation_max_file_size(max_file_size);
                                                     rfh_cfg.set_max_backup_files(1);
                                                     rfh_cfg.set_overwrite_rolled_files(true);
                                                     rfh_cfg.set_open_mode('w');
                                                     return rfh_cfg;
                                                   }());

  quill::Logger* logger_b = Frontend::create_or_get_logger("logger_b", std::move(rotating_file_sink_b));

  // log a few messages so we rotate files
  for (uint32_t i = 0; i < 20; ++i)
  {
    LOG_INFO(logger_a, "Hello rotating file log num {}", i);
    LOG_INFO(logger_b, "Hello rotating file log num {}", i);
  }

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(base_filename_a);
  REQUIRE_GE(file_contents.size(), 3);

  std::vector<std::string> const file_contents_1 = quill::testing::file_contents(base_filename_a_1);
  REQUIRE_GE(file_contents_1.size(), 7);

  std::vector<std::string> const file_contents_2 = quill::testing::file_contents(base_filename_a_2);
  REQUIRE_GE(file_contents_2.size(), 7);

  // File from 2nd logger
  std::vector<std::string> const file_contents_3 = quill::testing::file_contents(base_filename_b);
  REQUIRE_GE(file_contents_3.size(), 4);

  std::vector<std::string> const file_contents_4 = quill::testing::file_contents(base_filename_b_1);
  REQUIRE_GE(file_contents_4.size(), 7);

  // Remove filenames
  testing::remove_file(base_filename_a);
  testing::remove_file(base_filename_a_1);
  testing::remove_file(base_filename_a_2);
  testing::remove_file(base_filename_b);
  testing::remove_file(base_filename_b_1);
}