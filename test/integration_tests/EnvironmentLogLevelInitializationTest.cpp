#include "doctest/doctest.h"

#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/core/QuillError.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdlib>
#include <memory>

using namespace quill;

namespace
{
#if !defined(QUILL_NO_EXCEPTIONS)
void set_log_level_env(char const* value)
{
  #if defined(_WIN32)
  _putenv_s("QUILL_LOG_LEVEL", value);
  #else
  setenv("QUILL_LOG_LEVEL", value, 1);
  #endif
}
#endif
} // namespace

/***/
TEST_CASE("invalid_environment_log_level_is_catchable_during_first_logger_creation")
{
#if defined(QUILL_NO_EXCEPTIONS)
  return;
#else
  // This test has its own executable so LoggerManager has not been initialized before the
  // environment is set.
  set_log_level_env("backtrace");
  std::shared_ptr<Sink> sink = std::make_shared<ConsoleSink>();

  REQUIRE_THROWS_AS((void)Frontend::create_logger("invalid_env_level_logger", sink), QuillError);
  REQUIRE_EQ(Frontend::get_number_of_loggers(), 0u);

  // A failed first parse must not poison singleton initialization or leave a partial logger.
  set_log_level_env("");
  Logger* logger = Frontend::create_logger("valid_env_level_retry_logger", std::move(sink));
  REQUIRE_NE(logger, nullptr);
  REQUIRE_EQ(logger->get_log_level(), LogLevel::Info);
#endif
}
