/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/handlers/StreamHandler.h" // for StreamHandler
#include <string>                         // for string

namespace quill
{

enum class FilenameAppend : uint8_t
{
  DateTime,
  Date,
  None
};

/**
 * Creates a new instance of the FileHandler class.
 * The specified file is opened and used as the stream for logging.
 * If mode is not specified, "a" is used.
 * By default, the file grows indefinitely.
 */
class FileHandler : public StreamHandler
{
public:
  /**
   * This constructor will always call fopen to open_file the given file
   * @param filename string containing the name of the file to be opened.
   * @param mode string containing a file access mode.
   * @param append_to_filename append extra info to filename
   * @param file_event_notifier notifies on file events
   * @param do_fsync also fsync when flushing
   */
  FileHandler(fs::path const& filename, std::string const& mode, FilenameAppend append_to_filename,
              FileEventNotifier file_event_notifier, bool do_fsync);

  ~FileHandler() override;

  /**
   * Flushes the stream and optionally fsyncs it
   */
  QUILL_ATTRIBUTE_HOT void flush() noexcept override;

protected:
  /**
   * This constructor will not call fopen. It will just store the filename as base_filename and is
   * used by FileHandlers that derive from this class e.g. DailyFileHandler. Those filehandlers
   * usually do not operate directly on the base_filename but instead they usually append
   * something to the filename and open_file it themselves
   * @param filename  string containing the base name of the files
   * @param append_to_filename append extra info to filename
   * @param file_event_notifier file event notifier
   * @param do_fsync also fsync when flushing
   */
  FileHandler(fs::path const& filename, FilenameAppend append_to_filename,
              FileEventNotifier file_event_notifier, bool do_fsync);

  void open_file(fs::path const& filename, std::string const& mode);
  void close_file();

private:
  bool _fsync{false};
};
} // namespace quill
