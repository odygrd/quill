#include "external_lib/quill_wrapper.h"

int main()
{
  QuillWrapper::instance().setup_log();
  QuillWrapper::get_logger()->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(QuillWrapper::get_logger(), "This is a log trace l3 example {}", 1);
  LOG_TRACE_L2(QuillWrapper::get_logger(), "This is a log trace l2 example {} {}", 2, 2.3);
  LOG_TRACE_L1(QuillWrapper::get_logger(), "This is a log trace l1 {} example", "string");
  LOG_DEBUG(QuillWrapper::get_logger(), "This is a log debug example {}", 4);
  LOG_INFO(QuillWrapper::get_logger(), "This is a log info example {}", 5);
  LOG_WARNING(QuillWrapper::get_logger(), "This is a log warning example {}", 6);
  LOG_ERROR(QuillWrapper::get_logger(), "This is a log error example {}", 7);
  LOG_CRITICAL(QuillWrapper::get_logger(), "This is a log critical example {}", 8);
}
