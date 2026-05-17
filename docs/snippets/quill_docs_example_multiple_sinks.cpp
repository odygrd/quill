#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <utility>

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto file_sink_a = quill::Frontend::create_or_get_sink<quill::FileSink>("sink_a.log");
  quill::Logger* logger_a = quill::Frontend::create_or_get_logger("logger_a", std::move(file_sink_a));

  auto file_sink_b = quill::Frontend::create_or_get_sink<quill::FileSink>("sink_b.log");
  quill::Logger* logger_b = quill::Frontend::create_or_get_logger("logger_b", std::move(file_sink_b));

  LOG_INFO(logger_a, "Hello from {}", "sink example");
  LOG_INFO(logger_a, "Using logger_a");

  LOG_INFO(logger_b, "Different data for sink B");
  LOG_INFO(logger_b, "Using logger_b");
}