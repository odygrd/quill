#pragma once

#include "quill/backend/PatternFormatter.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/sinks/RotatingFileSink.h"

#include <string>
#include <string_view>

class RotatingFileSinkWithFormatter : public quill::RotatingFileSink
{
public:
  RotatingFileSinkWithFormatter(quill::fs::path const& filename, quill::RotatingFileSinkConfig const& config,
                                std::string const& format_pattern, std::string const& time_format,
                                quill::Timezone timestamp_timezone = quill::Timezone::LocalTime,
                                quill::FileEventNotifier file_event_notifier = quill::FileEventNotifier{})
    : quill::RotatingFileSink(filename, config, file_event_notifier),
      _formatter(format_pattern, time_format, timestamp_timezone)
  {
  }

  void write_log(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp, std::string_view thread_id,
                                     std::string_view thread_name, std::string const& process_id,
                                     std::string_view logger_name, quill::LogLevel log_level,
                                     std::vector<std::pair<std::string, std::string>> const* named_args,
                                     std::string_view log_message, std::string_view) override
  {
    std::string_view const formatted_log_statement =
      _formatter.format(log_timestamp, thread_id, thread_name, process_id, logger_name,
                        quill::loglevel_to_string(log_level), *log_metadata, named_args, log_message);

    quill::RotatingFileSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id, logger_name,
                                       log_level, named_args, log_message, formatted_log_statement);
  }

private:
  quill::PatternFormatter _formatter;
};