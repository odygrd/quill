#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/Sink.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class MyMetricSink final : public quill::Sink
{
public:
  void write_log(quill::MacroMetadata const* /** log_metadata **/, uint64_t /** log_timestamp **/,
                 std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                 std::string const& /** process_id **/, std::string_view /** logger_name **/,
                 quill::LogLevel /** log_level **/, std::string_view /** log_level_description **/,
                 std::string_view /** log_level_short_code **/,
                 std::vector<std::pair<std::string, std::string>> const* /** named_args **/,
                 std::string_view /** log_message **/, std::string_view /** log_statement **/) override
  {
  }

  void flush_sink() noexcept override {}

  // Called on the backend worker thread for every published sample.
  void write_metric(quill::MetricMetadata const* metric_metadata, uint64_t /** log_timestamp **/,
                    std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                    std::string const& /** process_id **/, std::string_view /** logger_name **/, double value) override
  {
    // Export however you like: Prometheus, StatsD, OpenTelemetry, a socket...
    std::cout << metric_metadata->metric_name() << " = " << value << '\n';
  }
};

int main()
{
  quill::Backend::start();

  auto sink = quill::Frontend::create_or_get_sink<MyMetricSink>("my_metric_sink");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("metrics", std::move(sink));

  // Register metric metadata once. The returned pointer is stable for the program lifetime.
  quill::MetricMetadata const* requests_total = quill::Frontend::create_metric(
    "requests_total_post_200", "requests_total", {{"method", "POST"}, {"status", "200"}});

  // Publish samples from the hot path.
  logger->publish_metric(requests_total, 1.0);
}
