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
 * @return
 */
QUILL_ALWAYS_INLINE uint64_t rdtsc() noexcept { return __rdtsc(); }

/**
 * Portable gmtime_r or _s per operating system
 * @param restrict
 * @param restrict
 * @return
 */
tm* gmtime_rs(time_t const* timer, tm* buf);

/**
 * Portable localtime_r or _s per operating system
 * @param restrict
 * @param restrict
 * @return
 */
tm* localtime_rs(time_t const* timer, tm* buf);

/**
 * Sets the cpu affinity of the caller thread to the given cpu id
 * @param cpu the cpu_id to pin the caller thread
 * @note: cpu_id starts from zero
 * @throws if fails to set cpu affinity
 */
QUILL_ATTRIBUTE_COLD void set_cpu_affinity(uint16_t cpu_id);

/**
 * Sets the name of the caller thread
 * @param name the name of the thread
 * @throws
 */
QUILL_ATTRIBUTE_COLD void set_thread_name(char const* name);

/**
 * Returns the os assigned id of the thread
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD uint32_t get_thread_id() noexcept;

/**
 * Get's the page size
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t get_page_size() noexcept;

/**
 * Provides hints to OS for memory page usage
 * @param addr
 * @param len
 * @param advice
 * @return
 */
QUILL_ATTRIBUTE_COLD void madvice(void* addr, size_t len);

/**
 * Aligned alloc
 * @param alignment
 * @param size
 * @return
 */
QUILL_NODISCARD void* aligned_alloc(size_t alignment, size_t size);

/**
 * Aligned free
 * @param ptr
 */
void aligned_free(void* ptr) noexcept;

/**
 * Opens a file
 * @param fp
 * @param filename
 * @param mode
 * @return Valid FILE*
 * @throws
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD FILE* fopen(filename_t const& filename, std::string const& mode);

/**
 * Calculates the size of a file
 * @param file
 * @return
 */
QUILL_NODISCARD QUILL_ATTRIBUTE_COLD size_t fsize(FILE* file);

/**
 * Removes a file
 * @param filename
 */
QUILL_ATTRIBUTE_COLD int remove(filename_t const& filename) noexcept;

#if defined(_WIN32)
/**
 * Given aa wide character fmt memory buffer convert it to a memory buffer
 * @param mem_buffer [out]
 * @param w_mem_buffer [in]
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