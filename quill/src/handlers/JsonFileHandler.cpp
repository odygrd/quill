#include "quill/handlers/JsonFileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully

namespace quill
{
/***/
void JsonFileHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
{
  _json_message.clear();

  _json_message.append(fmt::format(
    R"({{ "timestamp": "{}", "file": "{}", "line": "{}", "thread_id": "{}", "logger": "{}", "level": "{}", "message": "{}")",
    _formatter->format_timestamp(std::chrono::nanoseconds{log_event.header.timestamp}),
    log_event.header.metadata->macro_metadata.filename(), log_event.header.metadata->macro_metadata.lineno(),
    log_event.thread_id, log_event.header.logger_details->name(),
    quill::to_string(log_event.header.metadata->macro_metadata.level()),
    log_event.header.metadata->macro_metadata.message_format()));

  for (size_t i = 0; i < log_event.structured_keys.size(); ++i)
  {
    _json_message.append(
      fmt::format(R"(, "{}": "{}")", log_event.structured_keys[i], log_event.structured_values[i]));
  }

  _json_message.append(std::string_view{" }\n"});

  detail::fwrite_fully(_json_message.data(), sizeof(char), _json_message.size(), _file);
}
} // namespace quill