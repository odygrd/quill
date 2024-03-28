/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/handlers/Handler.h"
#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>

#include "quill/core/FileUtilities.h" // for fwrite_fully
#include <utility>                      // for move

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
                         FileEventNotifier file_event_notifier = FileEventNotifier{})
    : _filename(std::move(stream)), _file(file), _file_event_notifier(std::move(file_event_notifier))
  {
    // reserve stdout and stderr as filenames
    if (_filename == std::string{"stdout"})
    {
      _file = stdout;
    }
    else if (_filename == std::string{"stderr"})
    {
      _file = stderr;
    }
    else if (_filename == std::string{"/dev/null"})
    {
      _is_null = true;
    }
    else
    {
      // first attempt to create any non-existing directories
      std::error_code ec;
      fs::path parent_path;

      if (!_filename.parent_path().empty())
      {
        parent_path = _filename.parent_path();
        fs::create_directories(parent_path, ec);
        if (ec)
        {
          // use .string() to also support experimental fs
          QUILL_THROW(QuillError{fmtquill::format("cannot create directories for {}, error: {}",
                                                  parent_path.string(), ec.message())});
        }
      }
      else
      {
        parent_path = fs::current_path();
      }

      // convert the parent path to an absolute path
      fs::path const canonical_path = fs::canonical(parent_path, ec);

      if (ec)
      {
        // use .string() to also support experimental fs
        QUILL_THROW(QuillError{fmtquill::format("cannot make canonical path for {}, error: {}",
                                                parent_path.string(), ec.message())});
      }

      // finally replace the given filename's parent_path with the equivalent canonical path
      _filename = canonical_path / _filename.filename();
    }
  }

  ~StreamHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event log_event
   */
  QUILL_ATTRIBUTE_HOT void write(FormatBuffer const &formatted_log_message, TransitEvent const &log_event) override
  {
    if (QUILL_UNLIKELY(!_file))
    {
      // FileHandler::flush() tries to re-open a deleted file and if it fails _file can be null
      return;
    }

    if (_file_event_notifier.before_write)
    {
      std::string const modified_message = _file_event_notifier.before_write(
        std::string_view{formatted_log_message.data(), formatted_log_message.size()});

      detail::fwrite_fully(modified_message.data(), sizeof(char), modified_message.size(), _file);
    }
    else
    {
      detail::fwrite_fully(formatted_log_message.data(), sizeof(char), formatted_log_message.size(), _file);
    }

    _write_occurred = true;
  }

  /**
   * Flushes the stream
   */
  QUILL_ATTRIBUTE_HOT void flush() override
  {
    if (!_write_occurred || !_file)
    {
      return;
    }

    _write_occurred = false;
    fflush(_file);
  }

  /**
   * @return return the name of the file
   */
  QUILL_NODISCARD virtual fs::path const& filename() const noexcept { return _filename; }

  /**
   * @return stdout, stderr or file based on FILE*
   */
  QUILL_NODISCARD StreamHandlerType stream_handler_type() const noexcept
  {
    if (_file == stdout)
    {
      return StreamHandlerType::Stdout;
    }

    if (_file == stderr)
    {
      return StreamHandlerType::Stderr;
    }

    return StreamHandlerType::File;
  }

  QUILL_NODISCARD bool is_null() const noexcept { return _is_null; }

protected:
  fs::path _filename;
  FILE* _file{nullptr};
  FileEventNotifier _file_event_notifier;
  bool _is_null{false};
  bool _write_occurred{false};
};
} // namespace quill
