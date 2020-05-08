#include "quill/detail//HandlerCollection.h"
#include "quill/detail/Config.h"
#include "quill/detail/LoggerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/handlers/Handler.h"
#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(Logger, log_level)
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{tc, hc};

  Logger* logger = logger_collection.create_logger("logger_1");

  // Check default log level
  EXPECT_EQ(logger->log_level(), LogLevel::Info);

  // Change the log level
  logger->set_log_level(LogLevel::TraceL2);
  EXPECT_EQ(logger->log_level(), LogLevel::TraceL2);
}

/***/
TEST(Logger, get_non_existent_logger)
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{tc, hc};

#if defined(QUILL_NO_EXCEPTIONS)
  #if !defined(_WIN32)
  ASSERT_EXIT((void)logger_collection.get_logger("logger_1"), ::testing::KilledBySignal(SIGABRT),
              ".*");
  #endif
#else
  EXPECT_THROW((void)logger_collection.get_logger("logger_1"), quill::QuillError);
#endif
}

/***/
TEST(Logger, logger_should_log)
{
  Config cfg;
  ThreadContextCollection tc{cfg};
  HandlerCollection hc;

  LoggerCollection logger_collection{tc, hc};

  QUILL_MAYBE_UNUSED Logger* logger_1 = logger_collection.create_logger("logger_1");

  {
    LogLevel log_statement_level{LogLevel::Debug};
    EXPECT_FALSE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  {
    LogLevel log_statement_level{LogLevel::Info};
    EXPECT_TRUE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  {
    LogLevel log_statement_level{LogLevel::Error};
    EXPECT_TRUE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  // change log level
  logger_collection.get_logger("logger_1")->set_log_level(LogLevel::TraceL3);

  {
    LogLevel log_statement_level{LogLevel::TraceL3};
    EXPECT_TRUE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  {
    LogLevel log_statement_level{LogLevel::Critical};
    EXPECT_TRUE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  // change log level
  logger_collection.get_logger("logger_1")->set_log_level(LogLevel::None);

  {
    LogLevel log_statement_level{LogLevel::TraceL3};
    EXPECT_FALSE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }

  {
    LogLevel log_statement_level{LogLevel::Critical};
    EXPECT_FALSE(logger_collection.get_logger("logger_1")->should_log(log_statement_level));
  }
}