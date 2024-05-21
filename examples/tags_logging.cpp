#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/Utility.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

/**
 * This example showcases the usage of the _WITH_TAGS macros, which allow for logging additional
 * tags with each log message.
 *
 * The example demonstrates the construction of custom tags at compile time using
 * the quill::CustomTags interface.
 *
 * One important limitation to note is that quill::CustomTags instances must be constructed
 * at compile time.
 */

class MyTagsA : public quill::Tags
{
public:
  constexpr MyTagsA(char const* tag_a, uint32_t tag_b) : _tag_a(tag_a), _tag_b(tag_b) {}

  void format(std::string& out) const override
  {
    out.append(_tag_a);
    out.append(":");
    out.append(std::to_string(_tag_b));
  }

private:
  char const* _tag_a;
  uint32_t _tag_b;
};

// Define another CustomTags class
class MyTagsB : public quill::Tags
{
public:
  constexpr MyTagsB(char const* tag_a, uint32_t tag_b) : _tag_a(tag_a), _tag_b(tag_b) {}

  void format(std::string& out) const override
  {
    out.append(_tag_a);
    out.append(":");
    out.append(std::to_string(_tag_b));
  }

private:
  char const* _tag_a;
  uint32_t _tag_b;
};

static constexpr MyTagsA tags_a{"CUSTOM_TAG_A", 12};
static constexpr MyTagsB tags_b{"CUSTOM_TAG_B", 23};

// Combine different tags
static constexpr quill::utility::CombinedTags<MyTagsA, MyTagsB> tags_ab{tags_a, tags_b};

static constexpr quill::utility::CombinedTags<MyTagsA, MyTagsB> tags_ab_custom_format_delimiter{
  tags_a, tags_b, " -- "};

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // It is important to change the default logging pattern to include the custom tags
  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    "%(time) [%(thread_id)] %(short_source_location:<28) %(log_level:<9) %(logger:<16) "
    "[%(custom_tags)] %(message)",
    "%Y-%m-%d %H:%M:%S.%Qms", quill::Timezone::GmtTime);

  LOG_TRACE_L3_WITH_TAGS(logger, tags_a, "TraceL3 with custom tags");
  LOG_TRACE_L2_WITH_TAGS(logger, tags_b, "TraceL2 with custom tags");
  LOG_TRACE_L1_WITH_TAGS(logger, tags_b, "TraceL1 with custom tags");
  LOG_DEBUG_WITH_TAGS(logger, tags_a, "Debug with custom tags");
  LOG_INFO_WITH_TAGS(logger, tags_a, "Info with custom tags");
  LOG_WARNING_WITH_TAGS(logger, tags_b, "Warning with custom tags");
  LOG_ERROR_WITH_TAGS(logger, tags_a, "Error with custom tags");

  LOG_INFO_WITH_TAGS(logger, tags_ab, "Info with combined custom tags");

  LOG_INFO_WITH_TAGS(logger, tags_ab_custom_format_delimiter,
                     "Combined custom tags custom delimiter");

  LOG_INFO(logger, "Without custom tags");
}
