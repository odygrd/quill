#include "quill/handlers/RotatingFileHandler.h"
#include "quill/QuillError.h"                // for QUILL_THROW, QuillError
#include "quill/detail/misc/Common.h"        // for QUILL_UNLIKELY
#include "quill/detail/misc/FileUtilities.h" // for append_index_to_filename
#include "quill/detail/misc/Os.h"            // for rename_file
#include "quill/handlers/StreamHandler.h"    // for StreamHandler
#include <cerrno>                            // for errno
#include <ostream>                           // for operator<<, basic_ostre...

namespace quill
{
/***/
RotatingFileHandler::RotatingFileHandler(fs::path const& base_filename, std::string const& mode,
                                         size_t max_bytes, uint32_t backup_count,
                                         bool overwrite_oldest_files, bool clean_old_files,
                                         FileEventNotifier file_event_notifier, bool do_fsync)
  : FileHandler(base_filename, std::move(file_event_notifier), do_fsync),
    _max_bytes(max_bytes),
    _backup_count(backup_count),
    _overwrite_oldest_files(overwrite_oldest_files)
{
  // if we are starting in w mode, then we also should clean all previous log files of the previous run
  if (clean_old_files && (mode == "w"))
  {
    for (const auto& entry : fs::directory_iterator(fs::current_path() / base_filename.parent_path()))
    {
      std::size_t found = entry.path().string().find(base_filename.stem().string() + ".");
      if (found != std::string::npos)
      {
        fs::remove(entry);
      }
    }
  }
  else if (mode == "a")
  {
    // Since we are appending, we need to find the current index
    for (const auto& entry : fs::directory_iterator(fs::current_path() / base_filename.parent_path()))
    {
      std::size_t found = entry.path().string().find(base_filename.stem().string() + ".");
      if (found != std::string::npos)
      {
        // stem will be something like `logfile.1`
        size_t pos = entry.path().stem().string().find_last_of('.');
        if (pos != std::string::npos)
        {
          std::string index =
            entry.path().stem().string().substr(pos + 1, entry.path().stem().string().length());

          // Attempt to convert the index to a number
          QUILL_TRY
          {
            auto index_num = static_cast<uint32_t>(std::stoul(index));
            _current_index = (std::max)(_current_index, index_num);
            if (_current_index > (_backup_count - 1))
            {
              // don't increment past _backup_count
              _current_index = (_backup_count - 1);
            }
          }
          QUILL_CATCH_ALL() { continue; }
        }
      }
    }
  }

  open_file(_filename, mode);
  _current_size = detail::file_size(_filename);
}

/***/
void RotatingFileHandler::write(fmt_buffer_t const& formatted_log_message, quill::TransitEvent const& log_event)
{
  size_t new_size = _current_size + formatted_log_message.size();

  if (new_size > _max_bytes)
  {
    // rotate when the new estimated file size exceeds max size.
    FileHandler::flush();

    if (detail::file_size(_filename) > 0)
    {
      // Also check the file size is > 0  to better deal with full disk
      _rotate();
      new_size = formatted_log_message.size();
    }
  }

  // write to file
  StreamHandler::write(formatted_log_message, log_event);
  _current_size = new_size;
}

/***/
void RotatingFileHandler::_rotate()
{
  if ((_current_index == _backup_count) && !_overwrite_oldest_files)
  {
    // we can not rotate anymore, do not rotate
    return;
  }

  // close the previous file
  close_file();

  // if we have more than 2 files we need to start renaming recursively
  for (uint32_t i = _current_index; i >= 1; --i)
  {
    fs::path const previous_file = detail::append_index_to_filename(_filename, i);
    fs::path const new_file = detail::append_index_to_filename(_filename, i + 1);
    quill::detail::rename_file(previous_file, new_file);
  }

  if (_backup_count > 0)
  {
    // then we will always rename_file the base filename to 1
    fs::path const previous_file = _filename;
    fs::path const new_file = detail::append_index_to_filename(_filename, 1);
    quill::detail::rename_file(previous_file, new_file);

    if (!_overwrite_oldest_files)
    {
      // always increment current index as we will break above. This is needed to avoid
      // _backup_count - 1 contidion below when _backup_count == 1 and !_overwrite_oldest_files
      ++_current_index;
    }
    else
    {
      // Increment the rotation index, -1 as we are counting _current_index from 0
      if (_current_index < (_backup_count - 1))
      {
        // don't increment past _backup_count
        ++_current_index;
      }
    }
  }

  // Now reopen the base filename for writing again
  open_file(_filename, "w");
}
} // namespace quill