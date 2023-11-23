/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Attributes.h" // for QUILL_ATTRIBUTE_COLD, QUIL...
#include "quill/detail/misc/Common.h"     // for fs::path
#include <cstdint>                        // for uint32_t, uint16_t
#include <cstdio>                         // for FILE
#include <ctime>                          // for size_t, time_t
#include <string>  // for string
#include <utility> // for pair

/** forward declarations **/
struct tm;

namespace quill::detail
{
#if defined(_WIN32)
/**
 * Return the size required to encode a wide string
 * @param s wide string to be encoded
 * @return required size for encoding
 */
size_t get_wide_string_encoding_size(std::wstring_view s);

/**
 * Converts a wide string to a narrow string
 */
void wide_string_to_narrow(void* dest, size_t required_bytes, std::wstring_view s);
#endif

 /**
 * Portable gmtime_r or _s per operating system
 * @param timer to a time_t object to convert
 * @param buf to a struct tm object to store the result
 * @return copy of the buf pointer, or throws on error
 * @throws std::system_error
 */
tm* gmtime_rs(time_t const* timer, tm* buf);

/**
 * Portable localtime_r or _s per operating system
 * @param timer to a time_t object to convert
 * @param buf to a struct tm object to store the result
 * @return copy of the buf pointer, or throws on error
 * @throws std::system_error
 */
tm* localtime_rs(time_t const* timer, tm* buf);

/**
 * Sets the cpu affinity of the caller thread to the given cpu id
 * @param cpu_id the cpu_id to pin the caller thread
 * @note: cpu_id starts from zero
 * @throws if fails to set cpu affinity
 */
QUILL_ATTRIBUTE_COLD void set_cpu_affinity(uint16_t cpu_id);

/**
 * Sets the name of the caller thread
 * @param name the name of the thread
 * @throws std::runtime_error if it fails to set cpu affinity
 */
QUILL_ATTRIBUTE_COLD void set_thread_name(char const* name);

/**
 * Returns the name of the thread. By default, each thread is unnamed.
 * If set_thread_name has not been used to set the name of the specified thread,
 * a null string is retrieved into name.
 * @return the thread name
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::string get_thread_name();

/**
 * Returns the os assigned ID of the thread
 * @return the thread ID of the calling thread
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD uint32_t get_thread_id() noexcept;

/**
 * Returns the os assigned ID of the process
 * @return the process ID of the calling process
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD uint32_t get_process_id() noexcept;

/**
 * Aligned alloc
 * @param size number of bytes to allocate. An integral multiple of alignment
 * @param alignment specifies the alignment. Must be a valid alignment supported by the implementation.
 * @param huge_pages allocate huge pages, only suported on linux
 * @return On success, returns the pointer to the beginning of newly allocated memory.
 * To avoid a memory leak, the returned pointer must be deallocated with free_aligned().
 * @throws  std::system_error on failure
 */

QUILL_NODISCARD void* alloc_aligned(size_t size, size_t alignment, bool huge_pages = false);

/**
 * Free aligned memory allocated with alloc_aligned
 * @param ptr address to aligned memory
 */
void free_aligned(void* ptr) noexcept;

/**
 * inverses of gmtime
 * @param tm struct tm to convert
 * @throws on invalid input
 */
QUILL_ATTRIBUTE_COLD time_t timegm(tm* tm);

/**
 * Check if the terminal supports colours
 * @return true if the terminate supports colours
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD bool is_colour_terminal() noexcept;

/**
 * Check if file descriptor is attached to terminal
 * @param file the file handler
 * @return true if the file is attached to terminal
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD bool is_in_terminal(FILE* file) noexcept;

/**
 * fsync the file descriptor
 * @param fd file
 */
bool fsync(FILE* fd);
} // namespace quill::detail
