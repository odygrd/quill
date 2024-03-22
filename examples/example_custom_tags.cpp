#include "quill/Quill.h"
#include "quill/Utility.h"

#include <chrono>
#include <thread>

// Define a CustomTags class
class MyCustomTagsA : public quill::CustomTags
{
public:
  constexpr MyCustomTagsA(char const* tag_a, uint32_t tag_b) : _tag_a(tag_a), _tag_b(tag_b) {}

  void format(std::string& out) const override
  {
    out.append(fmtquill::format("{}:{}", _tag_a, _tag_b));
  }

private:
  char const* _tag_a;
  uint32_t _tag_b;
};

// Define another CustomTags class
class MyCustomTagsB : public quill::CustomTags
{
public:
  constexpr MyCustomTagsB(char const* tag_a, uint32_t tag_b) : _tag_a(tag_a), _tag_b(tag_b) {}

  void format(std::string& out) const override
  {
    out.append(fmtquill::format("{}:{}", _tag_a, _tag_b));
  }

private:
  char const* _tag_a;
  uint32_t _tag_b;
};

static constexpr MyCustomTagsA custom_tags_a{"CUSTOM_TAG_A", 12};
static constexpr MyCustomTagsB custom_tags_b{"CUSTOM_TAG_B", 23};

// Combine different tags
static constexpr quill::utility::CombinedCustomTags<MyCustomTagsA, MyCustomTagsB> custom_tags_ab{
  custom_tags_a, custom_tags_b};

static constexpr quill::utility::CombinedCustomTags<MyCustomTagsA, MyCustomTagsB> custom_tags_ab_custom_format_delimiter{
  custom_tags_a, custom_tags_b, " -- "};

int main()
{
  // Set a custom formatter for stdout that prints the tags
  std::shared_ptr<quill::Handler> stdout_handler = quill::stdout_handler();

  stdout_handler->set_pattern(
    "%(time) [%(thread_id)] %(short_source_location:<28) %(log_level) %(logger:<16) - "
    "[%(custom_tags)] "
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
  LOG_CRITICAL_WITH_TAGS(logger, custom_tags_ab, "Critical with combined custom tags");
  LOG_CRITICAL_WITH_TAGS(logger, custom_tags_ab_custom_format_delimiter,
                         "Critical with combined custom tags custom delimiter");
  LOG_CRITICAL(logger, "Critical without custom tags");
}
