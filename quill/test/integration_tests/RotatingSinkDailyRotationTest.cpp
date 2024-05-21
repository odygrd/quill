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
TEST_CASE("rotating_sink_daily_rotation")
{
  // This test has 2 loggers that they logged to different rotating file handlers
  static char const* base_filename = "rotating_sink_daily_rotation.log";

  // Start the logging backend thread
  Backend::start();

  // Create a rotating file handler
  std::shared_ptr<quill::Sink> rotating_file_sink_a =
    Frontend::create_or_get_sink<RotatingFileSink>(base_filename,
                                                   []()
                                                   {
                                                     RotatingFileSinkConfig rfh_cfg;
                                                     rfh_cfg.set_rotation_time_daily("00:00");
                                                     rfh_cfg.set_open_mode('w');
                                                     return rfh_cfg;
                                                   }());

  // Get the same instance back - we search it again (for testing only)
  quill::Logger* logger = Frontend::create_or_get_logger("logger", std::move(rotating_file_sink_a));

  // log a few messages so we rotate files
  for (uint32_t i = 0; i < 20; ++i)
  {
    LOG_INFO(logger, "Hello daily file log num {}", i);
  }

  // flush all log and remove all loggers
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(base_filename);
  REQUIRE_EQ(file_contents.size(), 20);

  testing::remove_file(base_filename);
}