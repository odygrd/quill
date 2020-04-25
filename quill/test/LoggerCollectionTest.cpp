#include "quill/detail/LoggerCollection.h"
#include "quill/detail/HandlerCollection.h"
#include "quill/detail/ThreadContextCollection.h"
#include "quill/handlers/StreamHandler.h"
#include <gtest/gtest.h>

using namespace quill;
using namespace quill::detail;

/***/
TEST(LoggerCollection, create_get_same_logger)
{
  // Create and then get the same logger and check that the values we set are cached
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Handler* stream_handler = hc.stdout_streamhandler();
  Logger* logger_1 = logger_collection.create_logger("logger_1", stream_handler);
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* logger_2 = logger_collection.get_logger("logger_1");
  EXPECT_EQ(logger_2->log_level(), LogLevel::TraceL2);
}

/***/
TEST(LoggerCollection, create_get_different_loggers)
{
  // Create and then get the same logger and check that the values we set are different
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Handler* stream_handler = hc.stdout_streamhandler();
  Logger* logger_1 = logger_collection.create_logger("logger_1", stream_handler);
  EXPECT_EQ(logger_1->log_level(), LogLevel::Info);

  // change existing log level
  logger_1->set_log_level(LogLevel::TraceL2);

  // try to get a new logger with a default log level
  Handler* stream_handler_2 = hc.stdout_streamhandler();
  QUILL_MAYBE_UNUSED Logger* logger_2 = logger_collection.create_logger("logger_2", stream_handler_2);
  Logger* logger_3 = logger_collection.get_logger("logger_2");
  EXPECT_EQ(logger_3->log_level(), LogLevel::Info);
}

/***/
TEST(LoggerCollection, get_non_existing_logger)
{
  // Check that we throw if we try to get a logger that was never created before
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  // try to get a new logger with a default log level
#if defined(QUILL_NO_EXCEPTIONS)
  #if !defined(_WIN32)
  ASSERT_EXIT(QUILL_MAYBE_UNUSED auto logger = logger_collection.get_logger("logger_2"),
              ::testing::KilledBySignal(SIGABRT), ".*");
  #endif
#else
  EXPECT_THROW(QUILL_MAYBE_UNUSED auto logger = logger_collection.get_logger("logger_2"), quill::QuillError);
#endif
}

/***/
TEST(LoggerCollection, default_logger)
{
  // Get the default logger and change the log level, then get the default logger again ans check
  // the values we set are cached
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Logger* default_logger = logger_collection.get_logger();
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);

  // change existing log level
  default_logger->set_log_level(LogLevel::TraceL2);

  // try to get the same logger again and check the log level is updated
  Logger* default_logger_2 = logger_collection.get_logger();
  EXPECT_EQ(default_logger_2->log_level(), LogLevel::TraceL2);

  // Get the default logger by name
  Logger* default_logger_3 = logger_collection.get_logger("root");
  EXPECT_EQ(default_logger_3->log_level(), LogLevel::TraceL2);
}

/***/
TEST(LoggerCollection, create_logger_from_default_logger)
{
  // Create a new logger and check that the properties are the same as the default logger
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Logger* default_logger = logger_collection.create_logger("logger_test");
  EXPECT_EQ(default_logger->log_level(), LogLevel::Info);
}

/***/
TEST(LoggerCollection, set_default_logger_handler)
{
  // Set a handler to the default logger
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Handler* stream_handler = hc.stdout_streamhandler();
  logger_collection.set_default_logger_handler(stream_handler);
}

/***/
TEST(LoggerCollection, set_default_logger_pattern)
{
  // Set a handler to the default logger with a default pattern
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  Handler* stream_handler = hc.stdout_streamhandler();
  stream_handler->set_pattern(QUILL_STRING("%(message)"));

  logger_collection.set_default_logger_handler(stream_handler);
}

/***/
TEST(LoggerCollection, set_default_logger_multiple_handlers)
{
  // Set a handler to the default logger with a default pattern
  Config cfg;
  HandlerCollection hc;
  ThreadContextCollection tc{cfg};
  LoggerCollection logger_collection{tc, hc};

  logger_collection.set_default_logger_handler({hc.stdout_streamhandler(), hc.stderr_streamhandler()});
}
