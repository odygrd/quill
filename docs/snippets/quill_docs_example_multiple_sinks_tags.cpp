#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <utility>

#define SINK_A_TAG "sink_a"
#define SINK_B_TAG "sink_b"

class TagsFilter : public quill::Filter
{
public:
  explicit TagsFilter(std::string tag) : quill::Filter("tags_filter"), _tag(std::move(tag))
  {
    // logging library adds by default an extra char and space to each tag, so we also add it manually for the correct comparison
    _tag.insert(0, "#");
    _tag.push_back(' ');
  };

  bool filter(quill::MacroMetadata const* log_metadata, uint64_t /** log_timestamp **/,
              std::string_view /** thread_id **/, std::string_view /** thread_name **/,
              std::string_view /** logger_name **/, quill::LogLevel /** log_level **/,
              std::string_view /** log_message **/, std::string_view /** log_statement **/) noexcept override
  {
    return log_metadata->tags() && (strcmp(log_metadata->tags(), _tag.data()) == 0);
  }

private:
  std::string _tag;
};

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  auto file_sink_a = quill::Frontend::create_or_get_sink<quill::FileSink>("sink_a.log");
  file_sink_a->add_filter(std::make_unique<TagsFilter>(SINK_A_TAG));

  auto file_sink_b = quill::Frontend::create_or_get_sink<quill::FileSink>("sink_b.log");
  file_sink_b->add_filter(std::make_unique<TagsFilter>(SINK_B_TAG));

  quill::Logger* logger =
    quill::Frontend::create_or_get_logger("root", {std::move(file_sink_a), std::move(file_sink_b)});

  LOG_INFO_TAGS(logger, TAGS(SINK_A_TAG), "Hello from {}", "sink example");
  LOG_INFO_TAGS(logger, TAGS(SINK_A_TAG), "Using sink_a");

  LOG_INFO_TAGS(logger, TAGS(SINK_B_TAG), "Different data for sink B");
  LOG_INFO_TAGS(logger, TAGS(SINK_B_TAG), "Using sink_b");
}