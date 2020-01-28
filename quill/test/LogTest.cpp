#include <gtest/gtest.h>

#define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3

#include "TestUtility.h"
#include "quill/Macros.h"
#include "quill/detail/LogManager.h"
#include "quill/sinks/BasicFileSink.h"

/**
 * Contains tests that include frontend and backend thread testing
 * In real logger usage LogManager would be a singleton
 */
using namespace quill;
using namespace quill::detail;

static Config default_cfg;

TEST(Log, custom_default_logger)
{
  std::string const filename{"test_custom_default_logger"};
  // Set a file sink as the custom logger sink and log to it
  LogManager lm{default_cfg};

  lm.logger_collection().set_custom_default_logger(std::make_unique<BasicFileSink>(filename));

  lm.start_backend_worker();

  Logger* default_logger = lm.logger_collection().get_logger();

  LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
  // LOG_ERROR(default_logger, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

  // TODO: Flush

  EXPECT_EQ(quill::testing::file_contents(filename),
            std::string{"Lorem ipsum dolor sit amet, consectetur adipiscing elit"});

  lm.stop_backend_worker();
}

TEST(Log, custom_pattern_default_logger)
{
  std::string const filename{"test_custom_pattern_default_logger"};
  // Set a file sink as the custom logger sink with a custom pattern and log to it
  LogManager lm{default_cfg};

  lm.logger_collection().set_custom_default_logger(
    std::make_unique<BasicFileSink>(filename, QUILL_STRING("%(message) %(function_name)")));

  lm.start_backend_worker();

  Logger* default_logger = lm.logger_collection().get_logger();

  LOG_INFO(default_logger, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
  LOG_ERROR(default_logger, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante");

  // TODO: Flush

  //  EXPECT_EQ(quill::testing::file_contents(filename),
  //            std::string{"Lorem ipsum dolor sit amet, consectetur adipiscing elit"});

  lm.stop_backend_worker();
}

// TODO :: Add multiple sinks

// TODO:: Add multiple sinks, custom format

// TODO:: Add stress test