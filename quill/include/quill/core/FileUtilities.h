/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/bundled/fmt/format.h"
#include "quill/core/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/core/Common.h"
#include "quill/core/Os.h"

#include <chrono>  // for system_clock, system_clock...
#include <cstdint> // for uint32_t
#include <cstdio>  // for FILE, size_t
#include <iostream>
#include <string>  // for string
#include <utility> // for pair

namespace quill::detail
{

/**
 * Simple wrapper around fwrite, throws on error
 * @param ptr Pointer to the array of elements to be written, converted to a const void*.
 * @param size Size in bytes of each element to be written.
 * @param count Number of elements, each one with a size of size bytes.
 * @param stream Pointer to a FILE object that specifies an output stream.
 */
QUILL_ATTRIBUTE_HOT inline void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream)
{
  size_t const written = std::fwrite(ptr, size, count, stream);

  if (QUILL_UNLIKELY(written < count))
  {
    QUILL_THROW(QuillError{fmtquill::format("fwrite failed [errno: {}, error: {}]", errno, strerror(errno))});
  }
}

/**
 * Opens a file
 * @param filename name of file
 * @param mode string containing a file access mode
 * @return a FILE* pointer to opened file
 * @throws std::system_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline FILE* open_file(fs::path const& filename, std::string const& mode)
{
  if (!filename.parent_path().empty())
  {
    std::error_code ec;
    fs::create_directories(filename.parent_path(), ec);
    if (ec)
    {
      // use .string() to also support experimental fs
      QUILL_THROW(QuillError{fmtquill::format("create directories failed [path: {}, error: {}]",
                                              filename.parent_path().string(), ec.message())});
    }
  }

  FILE* fp = fopen(filename.string().data(), mode.data());

  if (!fp)
  {
    QUILL_THROW(QuillError{fmtquill::format(
            "fopen failed. [path: {}, mode: {}, errno: {}, error: {}]", filename.string(), mode, errno,
            strerror(errno))});
  }
  return fp;
}

/**
 * Calculates the size of a file
 * @param filename a valid pointer to the file
 * @return the size of the file
 * @throws std::runtime_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline size_t file_size(fs::path const& filename)
{
  return static_cast<size_t>(fs::file_size(filename));
}

/**
 * Removes a file
 * @param filename the name of the file to remove_file
 * @return True if the file is removed, false otherwise
 */
QUILL_ATTRIBUTE_COLD inline bool remove_file(fs::path const& filename) noexcept
{
  std::error_code ec;
  fs::remove(filename, ec);

  if (ec)
  {
    return false;
  }

  return true;
}

/**
 * Renames a file
 * @param previous_file the old name of the file
 * @param new_file the new name of the file
 * @return True if the file is renamed, false otherwise
 */
bool inline rename_file(fs::path const& previous_file, fs::path const& new_file) noexcept
{
  std::error_code ec;
  fs::rename(previous_file, new_file, ec);

  if (ec)
  {
    std::cerr << "Failed to rename file from \"" << previous_file << "\" to \"" << new_file
              << "\" error: " << ec.message() << std::endl;
    return false;
  }

  return true;
}

/**
 * Return the main body of the filename and it's extension
 * @param filename the name of the file
 * @return first filename, second extension
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline std::pair<std::string, std::string> extract_stem_and_extension(
  fs::path const& filename) noexcept
{
  // filename and extension
  return std::make_pair((filename.parent_path() / filename.stem()).string(), filename.extension().string());
}

/**
 * Constructs a datetime string that can be used for filename naming
 * @param timestamp_ns timestamp
 * @param timezone timezone
 * @param with_time with_time
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline std::string get_datetime_string(uint64_t timestamp_ns,
                                                                            Timezone timezone, bool with_time)
{
  // convert to seconds
  auto const time_now = static_cast<time_t>(timestamp_ns / 1000000000);
  tm now_tm;

  if (timezone == Timezone::GmtTime)
  {
    gmtime_rs(&time_now, &now_tm);
  }
  else
  {
    localtime_rs(&time_now, &now_tm);
  }

  // Construct the string
  std::stringstream ss;
  if (with_time)
  {
    ss << std::setfill('0') << std::setw(4) << now_tm.tm_year + 1900 << std::setw(2)
       << now_tm.tm_mon + 1 << std::setw(2) << now_tm.tm_mday << "_" << std::setw(2)
       << now_tm.tm_hour << std::setw(2) << now_tm.tm_min << std::setw(2) << now_tm.tm_sec;
  }
  else
  {
    ss << std::setfill('0') << std::setw(4) << now_tm.tm_year + 1900 << std::setw(2)
       << now_tm.tm_mon + 1 << std::setw(2) << now_tm.tm_mday;
  }

  return ss.str();
}

/**
 * Append the date to the given filename
 * @param filename the name of the file
 * @param with_time also appends the time
 * @param timezone gmt or local time
 * @param timestamp optional, timestamp now, no timestamp gives current date
 * @return a filepath with the date appended
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline fs::path append_date_time_to_filename(
  fs::path const& filename, bool with_time = false, Timezone timezone = Timezone::LocalTime,
  std::chrono::system_clock::time_point timestamp = {}) noexcept
{
  // Get base file and extension
  auto const [stem, ext] = extract_stem_and_extension(filename);

  // Get the time now as tm from user or default to now
  std::chrono::system_clock::time_point const ts_now =
    (timestamp == std::chrono::system_clock::time_point{}) ? std::chrono::system_clock::now() : timestamp;

  uint64_t const timestamp_ns = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(ts_now.time_since_epoch()).count());

  // Construct a filename
  return fmtquill::format("{}_{}{}", stem, get_datetime_string(timestamp_ns, timezone, with_time), ext);
}

/**
 * Append an index to the given filename
 * @param filename the name of the file
 * @param index the index to append
 * @return a filepath with the index appended
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline fs::path append_index_to_filename(fs::path const& filename,
                                                                              uint32_t index) noexcept
{
  if (index == 0u)
  {
    return filename;
  }

  // Get base file and extension
  auto const [stem, ext] = extract_stem_and_extension(filename);

  // Construct a filename
  std::stringstream ss;
  ss << stem << "." << index << ext;
  return ss.str();
}

/**
 * Append an index to the given filename
 * @param filename the name of the file
 * @param text the text to append
 * @return a filepath with the index appended
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD inline fs::path append_string_to_filename(fs::path const& filename,
                                                                               std::string const& text) noexcept
{
  if (text.empty())
  {
    return filename;
  }

  // Get base file and extension
  auto const [stem, ext] = extract_stem_and_extension(filename);

  // Construct a filename
  std::stringstream ss;
  ss << stem << "." << text << ext;
  return ss.str();
}
} // namespace quill::detail
