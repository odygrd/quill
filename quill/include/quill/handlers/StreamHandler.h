/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"                    // for memory_buffer
#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_HOT, QUILL...
#include "quill/handlers/Handler.h"       // for Handler
#include <chrono>                         // for nanoseconds
#include <cstdio>                         // for FILE
#include <string>
#include <string_view>

namespace quill
{

/**
 * Notifies on file events by calling the appropriate callback, the callback is executed on
 * the backend worker thread
 */
struct FileEventNotifier
{
  std::function<void(fs::path const& filename)> before_open;
  std::function<void(fs::path const& filename, FILE* f)> after_open;
  std::function<void(fs::path const& filename, FILE* f)> before_close;
  std::function<void(fs::path const& filename)> after_close;
  std::function<std::string(std::string_view message)> before_write;
};

class StreamHandler : public Handler
{
public:
  enum class StreamHandlerType
  {
    Stdout,
    Stderr,
    File
  };

  /**
   * Constructor
   * Uses the default pattern formatter
   * @param stream only stdout or stderr
   * @param file file pointer
   * @param file_event_notifier notifies on file events
   * @throws on invalid param
   */
  explicit StreamHandler(fs::path stream, FILE* file = nullptr,
                         FileEventNotifier file_event_notifier = FileEventNotifier{});

  ~StreamHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event log_event
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message,
                                 quill::TransitEvent const& log_event) override;

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush() noexcept override;

  /**
   * @return return the name of the file
   */
  QUILL_NODISCARD virtual fs::path const& filename() const noexcept;

  /**
   * @return stdout, stderr or file based on FILE*
   */
  QUILL_NODISCARD StreamHandlerType stream_handler_type() const noexcept;

  QUILL_NODISCARD bool is_null() const noexcept;

protected:
  fs::path _filename;
  FILE* _file{nullptr};
  FileEventNotifier _file_event_notifier;
  bool _is_null{false};
};
} // namespace quill
