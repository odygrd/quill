/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include <chrono>
#include <string>

namespace quill
{
namespace detail
{
namespace file_utilities
{
/**
 * Simple wrapper around fwrite, throws on error
 * @param ptr
 * @param size
 * @param count
 * @param stream
 */
QUILL_ATTRIBUTE_HOT void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream);

/**
 * Opens a file
 * @param fp
 * @param filename
 * @param mode
 * @return Valid FILE*
 * @throws
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD FILE* open(filename_t const& filename, std::string const& mode);

/**
 * Get the size of a file
 * @param file
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t file_size(FILE* file);

/**
 * Removes a file
 * @param filename
 */
QUILL_ATTRIBUTE_COLD int remove(filename_t const& filename) noexcept;

/**
 * Return the main body of the filename and it's extension
 * @return first filename, second extension
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::pair<filename_t, filename_t> extract_stem_and_extension(filename_t const& filename) noexcept;

/**
 * Append the date to the given filename
 * @param filename
 * @param timestamp optional, timestamp now, no timestamp gives current date
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD filename_t
append_date_to_filename(filename_t const& base_filename,
                        std::chrono::system_clock::time_point timestamp = {}) noexcept;

/**
 * Append an index to the given filename
 * @param filename
 * @param timestamp optional, no timestamp gives current date
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD filename_t append_index_to_filename(filename_t const& base_filename,
                                                                         uint32_t index) noexcept;
} // namespace file_utilities
} // namespace detail
} // namespace quill