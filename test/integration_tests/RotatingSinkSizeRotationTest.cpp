#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/RotatingFileSink.h"
#include "quill/sinks/RotatingJsonFileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("rotating_sink_size_rotation")
{
  static char const* base_filename = "rotating_sink_size_rotation.log";
  static char const* base_filename_1 = "rotating_sink_size_rotation.1.log";
  static char const* base_filename_2 = "rotating_sink_size_rotation.2.log";

  static char const* json_filename = "json_rotating_sink_size_rotation.log";
  static char const* json_filename_1 = "json_rotating_sink_size_rotation.1.log";
  static char const* json_filename_2 = "json_rotating_sink_size_rotation.2.log";

  // Start the logging backend thread
  Backend::start();

  // Create a rotating file handler
  std::shared_ptr<Sink> rotating_file_sink_a =
    Frontend::create_or_get_sink<RotatingFileSink>(base_filename,
                                                   []()
                                                   {
                                                     RotatingFileSinkConfig rfh_cfg;
                                                     rfh_cfg.set_rotation_max_file_size(1024);
                                                     rfh_cfg.set_open_mode('w');
                                                     return rfh_cfg;
                                                   }());

  std::shared_ptr<Sink> json_rotating_file_sink_a =
    Frontend::create_or_get_sink<RotatingJsonFileSink>(json_filename,
                                                       []()
                                                       {
                                                         RotatingFileSinkConfig rfh_cfg;
                                                         rfh_cfg.set_rotation_max_file_size(1024);
                                                         rfh_cfg.set_open_mode('w');
                                                         return rfh_cfg;
                                                       }());

  // Get the same instance back - we search it again (for testing only)
  Logger* logger = Frontend::create_or_get_logger("logger", std::move(rotating_file_sink_a));
  Logger* json_logger = Frontend::create_or_get_logger("json_logger", std::move(json_rotating_file_sink_a));

  // log a few messages so we rotate files
  for (uint32_t i = 0; i < 20; ++i)
  {
    LOG_INFO(logger, "Hello daily file log num {}", i);
  }

  for (uint32_t i = 0; i < 12; ++i)
  {
    LOG_INFO(json_logger, "Hello daily file log num {num}", i);
  }

  // flush all log and remove all loggers
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = testing::file_contents(base_filename);
  REQUIRE_EQ(file_contents.size(), 4);

  std::vector<std::string> const file_contents_1 = testing::file_contents(base_filename_1);
  REQUIRE_EQ(file_contents_1.size(), 8);

  std::vector<std::string> const file_contents_2 = testing::file_contents(base_filename_2);
  REQUIRE_EQ(file_contents_2.size(), 8);

  std::vector<std::string> const file_contents_3 = testing::file_contents(json_filename);
  REQUIRE_EQ(file_contents_3.size(), 2);

  std::vector<std::string> const file_contents_4 = testing::file_contents(json_filename_1);
  REQUIRE_EQ(file_contents_4.size(), 5);

  std::vector<std::string> const file_contents_5 = testing::file_contents(json_filename_2);
  REQUIRE_EQ(file_contents_5.size(), 5);

  testing::remove_file(base_filename);
  testing::remove_file(base_filename_1);
  testing::remove_file(base_filename_2);
}