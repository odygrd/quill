#include "quill/handlers/JsonFileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully

namespace quill
{
/***/
void JsonFileHandler::write(fmt_buffer_t const& formatted_log_message, TransitEvent const& log_event)
{
  LogLevel const log_level = log_event.log_level();

  _json_message.clear();

  _json_message.append(fmtquill::format(
    R"({{"timestamp":"{}","file":"{}","line":"{}","thread_id":"{}","logger":"{}","level":"{}","message":"{}")",
    _formatter->format_timestamp(std::chrono::nanoseconds{log_event.timestamp}),
    log_event.macro_metadata->file_name(), log_event.macro_metadata->line(), log_event.thread_id,
    log_event.logger_details->name(), loglevel_to_string(log_level),
    log_event.macro_metadata->message_format()));

  for (auto const& [key, value] : log_event.structured_kvs)
  {
    _json_message.append(fmtquill::format(R"(,"{}":"{}")", key, std::string_view {value.data(), value.size()}));
  }

  _json_message.append(std::string_view{"}\n"});

  StreamHandler::write(_json_message, log_event);
}
} // namespace quill
