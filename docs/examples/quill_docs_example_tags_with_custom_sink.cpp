#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/Utility.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#define TAG_1 "foo"
#define TAG_2 "bar"
#define TAG_3 "baz"

class CustomSink final : public quill::Sink
{
public:
  CustomSink() = default;

  /***/
  void write_log(quill::MacroMetadata const* log_metadata, uint64_t /** log_timestamp **/,
                 std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                 std::string const& /** process_id **/, std::string_view /** logger_name **/,
                 quill::LogLevel /** log_level **/, std::string_view /** log_level_description **/,
                 std::string_view /** log_level_short_code **/,
                 std::vector<std::pair<std::string, std::string>> const* /** named_args - only populated when named args in the format placeholder are used **/,
                 std::string_view /** log_message **/, std::string_view log_statement) override
  {
    if (log_metadata->tags())
    {
      std::cout << "Message tags: " << log_metadata->tags() << std::endl;
    }

    std::cout << log_statement << std::endl;
  }

  /***/
  void flush_sink() noexcept override {}

  /***/
  void run_periodic_tasks() noexcept override {}
};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto sink = quill::Frontend::create_or_get_sink<CustomSink>("sink_id_1");

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) %(log_level:<9) "
      "%(message)",
      "%Y-%m-%d %H:%M:%S.%Qms", quill::Timezone::GmtTime});

  LOG_INFO_TAGS(logger, TAGS("random"), "Debug with tags");
  LOG_INFO_TAGS(logger, TAGS(TAG_2), "Info with tags");
  LOG_WARNING_TAGS(logger, TAGS(TAG_1, TAG_2), "Warning with tags");
  LOG_ERROR_TAGS(logger, TAGS(TAG_1, TAG_2, TAG_3), "Info with tags");

  LOG_INFO(logger, "Without tags");
}
