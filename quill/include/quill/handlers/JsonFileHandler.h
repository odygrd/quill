/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/FileHandler.h"

namespace quill
{

class JsonFileHandler : public FileHandler
{
public:
  JsonFileHandler(fs::path const& filename, std::string const& mode, FilenameAppend append_to_filename)
    : FileHandler(filename, mode, append_to_filename)
  {
    // JsonFileHandler requires an empty pattern
    set_pattern("");
  };

  ~JsonFileHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event log_event
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message,
                                 quill::TransitEvent const& log_event) override;

private:
  fmt_buffer_t _json_message;
};
} // namespace quill
