/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/FileHandler.h"

namespace quill
{

/**
 * The JsonFileHandlerConfig class holds the configuration options for the JsonFileHandler
 */
class JsonFileHandlerConfig : public FileHandlerConfig
{
public:
  JsonFileHandlerConfig() = default;
};

class JsonFileHandler : public FileHandler
{
public:
  JsonFileHandler(fs::path const& filename, JsonFileHandlerConfig const& config, FileEventNotifier file_event_notifier)
    : FileHandler(filename, static_cast<FileHandlerConfig const&>(config), std::move(file_event_notifier))
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
