/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/detail/misc/Common.h"     // for filename_t
#include <chrono>                         // for system_clock, system_clock...
#include <cstdint>                        // for uint32_t
#include <cstdio>                         // for FILE, size_t
#include <string>                         // for string
#include <utility>                        // for pair

namespace quill
{
namespace detail
{
namespace file_utilities
{

/**
 * Simple wrapper around fwrite, throws on error
 * @param ptr Pointer to the array of elements to be written, converted to a const void*.
 * @param size Size in bytes of each element to be written.
 * @param count Number of elements, each one with a size of size bytes.
 * @param stream Pointer to a FILE object that specifies an output stream.
 */
QUILL_ATTRIBUTE_HOT void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream);

/**
 * Opens a file
 * @param filename name of file
 * @param mode string containing a file access mode
 * @return a FILE* pointer to opened file
 * @throws std::system_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD FILE* open(filename_t const& filename, std::string const& mode);

/**
 * Calculates the size of a file
 * @param file a valid pointer to the file
 * @return the size of the file
 * @throws std::runtime_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t file_size(FILE* file);

/**
 * Removes a file
 * @param filename the name of the file to remove
 * @return ​Zero​ upon success or non-zero value on error.
 */
QUILL_ATTRIBUTE_COLD int remove(filename_t const& filename) noexcept;

/**
 * Return the main body of the filename and it's extension
 * @param filename the name of the file
 * @return first filename, second extension
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::pair<filename_t, filename_t> extract_stem_and_extension(filename_t const& filename) noexcept;

/**
 * Append the date to the given filename
 * @param filename the name of the file
 * @param timestamp optional, timestamp now, no timestamp gives current date
 * @param append_time also appends the time
 * @return a string as filename_t with the date appended
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD filename_t append_date_to_filename(
  filename_t const& filename, std::chrono::system_clock::time_point timestamp = {},
  bool append_time = false, bool utc = false) noexcept;

/**
 * Append an index to the given filename
 * @param filename the name of the file
 * @param index the index to append
 * @return a string as filename_t with the index appended
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD filename_t append_index_to_filename(filename_t const& filename,
                                                                         uint32_t index) noexcept;
} // namespace file_utilities
} // namespace detail
} // namespace quill