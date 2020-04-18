/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Fmt.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"
#include <cstdint>
#include <ctime>
#include <utility>

#if defined(_WIN32)
  #include <intrin.h>
#else
  #include <x86intrin.h>
#endif

namespace quill
{
namespace detail
{
/**
 * Get the TSC counter
 * @return rdtsc timestamp
 */
QUILL_ALWAYS_INLINE uint64_t rdtsc() noexcept { return __rdtsc(); }

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
 * Get's the page size
 * @return the size of the page
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t get_page_size() noexcept;

/**
 * Provides hints to OS for memory page usage
 * @param addr  begin address
 * @param len size of address
 * @throws  std::system_error on failure
 */
QUILL_ATTRIBUTE_COLD void madvice(void* addr, size_t len);

/**
 * Aligned alloc
 * @param alignment specifies the alignment. Must be a valid alignment supported by the implementation.
 * @param size number of bytes to allocate. An integral multiple of alignment
 * @return On success, returns the pointer to the beginning of newly allocated memory.
 * To avoid a memory leak, the returned pointer must be deallocated with aligned_free().
 * @throws  std::system_error on failure
 */
QUILL_NODISCARD void* aligned_alloc(size_t alignment, size_t size);

/**
 * Free aligned memory allocated with aligned_alloc
 * @param ptr address to aligned memory
 */
void aligned_free(void* ptr) noexcept;

/**
 * Opens a file
 * @param filename name of file
 * @param mode string containing a file access mode
 * @return a FILE* pointer to opened file
 * @throws std::system_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD FILE* fopen(filename_t const& filename, std::string const& mode);

/**
 * Calculates the size of a file
 * @param file a valid pointer to the file
 * @return the size of the file
 * @throws std::runtime_error on failure
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t fsize(FILE* file);

/**
 * Removes a file
 * @param filename the name of the file to remove
 * @return ​Zero​ upon success or non-zero value on error.
 */
QUILL_ATTRIBUTE_COLD int remove(filename_t const& filename) noexcept;

#if defined(_WIN32)
/**
 * Given a wide character fmt memory buffer convert it to a memory buffer
 * @param w_mem_buffer [in] wide buffer input
 * @param mem_buffer [out] output
 */
void wstring_to_utf8(fmt::wmemory_buffer const& w_mem_buffer, fmt::memory_buffer& mem_buffer);
#endif

/**
 * Creates the memory map files needed for the queue buffer
 * Maps the same region of physical memory two times into a contiguous virtual address range
 * first: valid address
 * second: file_handler (windows only)
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD std::pair<unsigned char*, void*> create_memory_mapped_files(size_t capacity);

/**
 * Destroys the memory mapped files that were created using create_memory_mapped_files
 */
QUILL_ATTRIBUTE_COLD void destroy_memory_mapped_files(std::pair<unsigned char*, void*> pointer_pair,
                                                      size_t capacity);

} // namespace detail
} // namespace quill