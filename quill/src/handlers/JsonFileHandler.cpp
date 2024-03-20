#include "quill/handlers/JsonFileHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully

namespace quill
{
/***/
void JsonFileHandler::write(fmt_buffer_t const& formatted_log_message, TransitEvent const& log_event)
{
  MacroMetadata const macro_metadata = log_event.metadata();
  LogLevel const log_level =
    log_event.log_level_override ? *log_event.log_level_override : macro_metadata.log_level();

  _json_message.clear();

  _json_message.append(fmtquill::format(
    R"({{"timestamp":"{}","file":"{}","line":"{}","thread_id":"{}","logger":"{}","level":"{}","message":"{}")",
    _formatter->format_timestamp(std::chrono::nanoseconds{log_event.header.timestamp}),
    macro_metadata.file_name(), macro_metadata.line(), log_event.thread_id,
    log_event.header.logger_details->name(), loglevel_to_string(log_level), macro_metadata.message_format()));

  for (auto const& [key, value] : log_event.structured_kvs)
  {
    _json_message.append(fmtquill::format(R"(,"{}":"{}")", key, std::string_view {value.data(), value.size()}));
  }

  _json_message.append(std::string_view{"}\n"});

  StreamHandler::write(_json_message, log_event);
}
} // namespace quill