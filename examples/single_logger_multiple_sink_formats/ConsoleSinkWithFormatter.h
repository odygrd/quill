#pragma once

#include "quill/backend/PatternFormatter.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/Filesystem.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <string_view>

class ConsoleSinkWithFormatter : public quill::ConsoleSink
{
public:
  ConsoleSinkWithFormatter(quill::PatternFormatterOptions const& pattern_formater_options,
                           bool enable_colours = true, std::string const& stream = "stdout")
    : quill::ConsoleSink(enable_colours, stream), _formatter(pattern_formater_options)
  {
  }

  void write_log(quill::MacroMetadata const* log_metadata, uint64_t log_timestamp,
                 std::string_view thread_id, std::string_view thread_name,
                 std::string const& process_id, std::string_view logger_name, quill::LogLevel log_level,
                 std::string_view log_level_description, std::string_view log_level_short_code,
                 std::vector<std::pair<std::string, std::string>> const* named_args,
                 std::string_view log_message, std::string_view) override
  {
    std::string_view const formatted_log_statement =
      _formatter.format(log_timestamp, thread_id, thread_name, process_id, logger_name, log_level_description,
                        log_level_short_code, *log_metadata, named_args, log_message);

    quill::ConsoleSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                                  logger_name, log_level, log_level_description, log_level_short_code,
                                  named_args, log_message, formatted_log_statement);
  }

private:
  quill::PatternFormatter _formatter;
};