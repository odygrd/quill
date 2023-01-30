#include "quill/handlers/JsonFileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully

namespace quill
{
/***/
void JsonFileHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
{
  std::pair<MacroMetadata, detail::FormatToFn> const mf = log_event.header.metadata_and_format_fn();
  MacroMetadata const& macro_metadata = mf.first;

  _json_message.clear();

  _json_message.append(fmt::format(
    R"({{ "timestamp": "{}", "file": "{}", "line": "{}", "thread_id": "{}", "logger": "{}", "level": "{}", "message": "{}")",
    _formatter->format_timestamp(std::chrono::nanoseconds{log_event.header.timestamp}),
    macro_metadata.filename(), macro_metadata.lineno(), log_event.thread_id,
    log_event.header.logger_details->name(), macro_metadata.level_as_str(), macro_metadata.message_format()));

  for (auto const& [key, value] : log_event.structured_kvs)
  {
    _json_message.append(fmt::format(R"(, "{}": "{}")", key, value));
  }

  _json_message.append(std::string_view{" }\n"});

  detail::fwrite_fully(_json_message.data(), sizeof(char), _json_message.size(), _file);
}
} // namespace quill