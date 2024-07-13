#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/JsonConsoleSink.h"

#include <string>

/**
 * This example showcases the usage of the JsonFileSink to generate JSON-formatted logs.
 * Additionally, it demonstrates how to simultaneously log in both the standard logger output
 * format, e.g., to console and the corresponding JSON format to a JSON output sink.
 *
 * For successful JSON logging, it's essential to use named placeholders within the provided
 * format string, such as "{method}" and "{endpoint}".
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend

  // Create a json sink
  auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonConsoleSink>("json_sink_1");

  // When logging json, it is ideal to set the logging pattern to empty to avoid unnecessary message formatting.
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "json_logger", std::move(json_sink), "", "%H:%M:%S.%Qns", quill::Timezone::GmtTime);

  int var_a = 123;
  std::string var_b = "test";

  // Log via the convenient LOGJ_ macros
  LOGJ_INFO(logger, "A json message", var_a, var_b);

  // Or manually specify the desired names of each variable
  LOG_INFO(logger, "A json message with {var_1} and {var_2}", var_a, var_b);
}
