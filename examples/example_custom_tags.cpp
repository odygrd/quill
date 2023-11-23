#include "quill/Quill.h"

#include <chrono>
#include <thread>

// Define a CustomTags class
class MyCustomTags : public quill::CustomTags
{
public:
  constexpr MyCustomTags(char const* tag_a, uint32_t tag_b) : _tag_a(tag_a), _tag_b(tag_b) {}

  void format(std::string& out) const override { out = fmtquill::format("{}:{}", _tag_a, _tag_b); }

private:
  char const* _tag_a;
  uint32_t _tag_b;
};

static constexpr MyCustomTags custom_tags_a{"CUSTOM_TAG_12", 12};
static constexpr MyCustomTags custom_tags_b{"CUSTOM_TAG_23", 23};

int main()
{
  // Set a custom formatter for stdout that prints the tags
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler();

  stdout_handler->set_pattern(
    "%(ascii_time) [%(thread)] %(fileline:<28) %(level_name) %(logger_name:<16) - [%(custom_tags)] "
    "%(message)",              // format
    "%Y-%m-%d %H:%M:%S.%Qms",  // timestamp format
    quill::Timezone::GmtTime); // timestamp's timezone

  // Register the handler as default
  quill::Config cfg;
  cfg.default_handlers.emplace_back(stdout_handler);

  // Apply the configuration
  quill::configure(cfg);
  quill::start();

  quill::Logger* logger = quill::get_logger();
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3_WITH_TAGS(logger, custom_tags_a, "TraceL3 with custom tags");
  LOG_TRACE_L2_WITH_TAGS(logger, custom_tags_b, "TraceL2 with custom tags");
  LOG_TRACE_L1_WITH_TAGS(logger, custom_tags_b, "TraceL1 with custom tags");
  LOG_DEBUG_WITH_TAGS(logger, custom_tags_a, "Debug with custom tags");
  LOG_INFO_WITH_TAGS(logger, custom_tags_a, "Info with custom tags");
  LOG_WARNING_WITH_TAGS(logger, custom_tags_b, "Warning with custom tags");
  LOG_ERROR_WITH_TAGS(logger, custom_tags_a, "Error with custom tags");
  LOG_CRITICAL_WITH_TAGS(logger, custom_tags_b, "Critical with custom tags");
  LOG_CRITICAL(logger, "Critical without custom tags");
}
