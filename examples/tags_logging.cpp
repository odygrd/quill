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

#define TAG_1 "foo"
#define TAG_2 "bar"
#define TAG_3 "baz"

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  // Ensure the logging pattern includes the %(tags) placeholder to display custom tags.
  // It is crucial to place %(tags) immediately before the next attribute without any intervening spaces.
  // This ensures correct formatting when no tags are present, preventing unwanted gaps in the output.

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    "root", std::move(console_sink),
    quill::PatternFormatterOptions{
      "%(time) [%(thread_id)] %(short_source_location:<28) %(log_level:<9) "
      "%(tags)%(message)",
      "%Y-%m-%d %H:%M:%S.%Qms", quill::Timezone::GmtTime});

  LOG_INFO_TAGS(logger, TAGS("random"), "Debug with tags");
  LOG_INFO_TAGS(logger, TAGS(TAG_2), "Info with tags");
  LOG_WARNING_TAGS(logger, TAGS(TAG_1, TAG_2), "Warning with tags");
  LOG_ERROR_TAGS(logger, TAGS(TAG_1, TAG_2, TAG_3), "Info with tags");

  LOG_INFO(logger, "Without tags");
}
