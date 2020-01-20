#include "quill/LogMacros.h"
#include "quill/detail/LogManager.h"

#include <gtest/gtest.h>

/***/
TEST(Log, basic_log)
{
  quill::detail::LogManager lm;

  lm.start_logging_worker();

  auto logger = lm.get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "test");
  LOG_TRACE_L2(logger, "test {}", 1);
  LOG_TRACE_L1(logger, "test {} {}", 1, 2);
  LOG_DEBUG(logger, "test a {2} b {1}  c {0}", 1, 2, 3);
  LOG_INFO(logger, "test a {} b {} c {} d {}", 1, 2, 3, 4);
  LOG_WARNING(logger, "test {}", "lol");
  LOG_ERROR(logger, "test {}", 122.3);
  LOG_CRITICAL(logger, "test");

  std::this_thread::sleep_for(std::chrono::seconds{2});
}