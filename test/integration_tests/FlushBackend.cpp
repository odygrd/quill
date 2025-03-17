#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

using namespace quill;

struct CustomFrontendOptions {
  static constexpr quill::QueueType queue_type =
    quill::QueueType::BoundedBlocking;
  static constexpr uint32_t initial_queue_capacity = 4 * 1024 * 1024;
  static constexpr uint32_t blocking_queue_retry_interval_ns = 800;
  static constexpr quill::HugePagesPolicy huge_pages_policy =
    quill::HugePagesPolicy::Try;
};

using CustomLogger = quill::LoggerImpl<CustomFrontendOptions>;
using CustomFrontend = quill::FrontendImpl<CustomFrontendOptions>;

struct QLogger {
  QLogger()
  {
    quill::Backend::start();
    auto console_sink =
      CustomFrontend::create_or_get_sink<quill::ConsoleSink>("global_console_sink");
    default_logger_ = CustomFrontend::create_or_get_logger("log_default_no_init", console_sink);
    user_logger_ = CustomFrontend::create_or_get_logger("log_user_no_init", console_sink);
  }

  ~QLogger()
  {
    flush_log();
    quill::Backend::stop();
  };

  void flush_log( uint32_t sleep_duration_ns = 100) const {
    default_logger_->flush_log(sleep_duration_ns);
  }

  CustomLogger* default_logger_;
  CustomLogger* user_logger_;
};

/***/
TEST_CASE("flush_flush_backend")
{
  for (int iter = 0; iter < 200; iter++)
  {
    QLogger test = QLogger();
    for (int i = 0; i < 100; i++)
    {
      LOG_INFO(test.default_logger_, "test {i}", i);
      test.flush_log();
    }
  }
}