#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/JsonSink.h"

#include <string>

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend

  // Create a json sink
  auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonConsoleSink>("json_sink_1");

  // PatternFormatter is only used for non-structured logs formatting
  // When logging only json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "json_logger", std::move(json_sink),
    quill::PatternFormatterOptions{"", "%H:%M:%S.%Qns", quill::Timezone::GmtTime});

  int var_a = 123;
  std::string var_b = "test";

  // Log via the convenient LOGJ_ macros
  LOGJ_INFO(logger, "A json message", var_a, var_b);

  // Or manually specify the desired names of each variable
  LOG_INFO(logger, "A json message with {var_1} and {var_2}", var_a, var_b);

  for (uint32_t i = 0; i < 40; ++i)
  {
    // Will only log the message once per second
    LOG_INFO_LIMIT(std::chrono::seconds{1}, logger, "A json message with {var_1} and {var_2}", var_a, var_b);
    LOGJ_INFO_LIMIT(std::chrono::seconds{1}, logger, "A json message", var_a, var_b);

    if (i % 10 == 0)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{(i / 10) * 500});
    }
  }

  for (uint32_t i = 0; i < 20; ++i)
  {
    // Will only log the message once per N occurrences second
    LOG_INFO_LIMIT_EVERY_N(10, logger, "A json message with {var_1} and {occurrence}", var_a, i);
    LOGJ_INFO_LIMIT_EVERY_N(10, logger, "A json message", var_a, i);
  }
}
