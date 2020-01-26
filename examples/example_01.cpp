#include "quill/Quill.h"

using namespace quill;

int main()
{
  // Start the logging backend
  quill::start_logging_worker();

  // a) We can use the default logger like this
  {
    // Using the default logger.
    // Default sink : stdout
    // Default LogLevel : Info
    // Default pattern : "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) %(logger_name) - %(message)"
    Logger* logger = quill::get_logger();

    // Change the LogLevel to print everything
    logger->set_log_level(quill::LogLevel::TraceL3);

    LOG_TRACE_L3(logger, "This is an log trace l3 example {}", 1);
    LOG_TRACE_L2(logger, "This is an log trace l2 example {} {}", 2, 2.3);
    LOG_TRACE_L1(logger, "This is an log trace l1 example {}", 3);
    LOG_DEBUG(logger, "This is an log debug example {}", 4);
    LOG_INFO(logger, "This is an log info example {}", 5);
    LOG_WARNING(logger, "This is an log warning example {}", 6);
    LOG_ERROR(logger, "This is an log error example {}", 7);
    LOG_CRITICAL(logger, "This is an log critical example {}", 8);
  }

  // b) Or like this
  {
    LOG_TRACE_L3(quill::get_logger(), "This is an log trace l3 example {}", 1);
    LOG_TRACE_L2(quill::get_logger(), "This is an log trace l2 example {} {}", 2, 2.3);
    LOG_TRACE_L1(quill::get_logger(), "This is an log trace l1 example {}", 3);
    LOG_DEBUG(quill::get_logger(), "This is an log debug example {}", 4);
    LOG_INFO(quill::get_logger(), "This is an log info example {}", 5);
    LOG_WARNING(quill::get_logger(), "This is an log warning example {}", 6);
    LOG_ERROR(quill::get_logger(), "This is an log error example {}", 7);
    LOG_CRITICAL(quill::get_logger(), "This is an log critical example {}", 8);
  }

  // c) Or like this
  {
#define DEF_LOG_INFO(fmt, ...) LOG_INFO(quill::get_logger(), fmt, ##__VA_ARGS__)

    quill::get_logger()->set_log_level(quill::LogLevel::Info);
    DEF_LOG_INFO("This is an log info example {}", 5);
  }
}