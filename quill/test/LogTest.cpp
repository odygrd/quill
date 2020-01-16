#include "quill/LogMacros.h"
#include "quill/detail/LogManager.h"

#include <gtest/gtest.h>

/***/
TEST(Log, basic_log)
{
  quill::detail::LogManager lm;

  auto logger = lm.get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "test");
  LOG_TRACE_L2(logger, "test", 1);
  LOG_TRACE_L1(logger, "test", 1, 2);
  LOG_DEBUG(logger, "test 123", 1, 2, 3);
  LOG_INFO(logger, "test", 1, 2, 3, 4);
  LOG_WARNING(logger, "test");
  LOG_ERROR(logger, "test");
  LOG_CRITICAL(logger, "test");
}