#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/AndroidSink.h"

#include <utility>

int main()
{
  quill::Backend::start();

  quill::AndroidSinkConfig sink_config;
  sink_config.set_tag("quill_android_compile_test");
  sink_config.set_format_message(true);

  auto android_sink = quill::Frontend::create_or_get_sink<quill::AndroidSink>(
    "android_sink", std::move(sink_config));

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "android_logger", std::move(android_sink), quill::PatternFormatterOptions{},
    quill::ClockSourceType::System);

  LOG_INFO(logger, "Android compile test {}", 1);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);
  quill::Backend::stop();
}
