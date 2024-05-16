/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/ConsoleHandler.h"

namespace quill
{
class JsonConsoleHandler : public ConsoleHandler
{
public:
  JsonConsoleHandler(std::string const& stream, FILE* file, ConsoleColours const& console_colours)
    : ConsoleHandler(stream, file, console_colours)
  {
    // JsonFileHandler requires an empty pattern
    set_pattern("");
  }

  ~JsonConsoleHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event log_event
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message, TransitEvent const& log_event) override;

private:
  fmt_buffer_t _json_message;
};
} // namespace quill
