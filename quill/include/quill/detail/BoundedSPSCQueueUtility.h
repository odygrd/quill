#pragma once

#include <utility>
#include <cstdint>

namespace quill
{
namespace detail
{
/**
 * Bounded SPPSCQueue is a templated class and we need to have all the implementation in the header file.
 * A lot of system files have to be included to perform the init function and the file mapping.
 * On windows we have to include windows.h but including windows.h in the header will include it everywhere
 * and will break all min and max macros.
 * To avoid defining NOMINMAX and WIN32_LEAN_AND_MEAN in the header we will abstract and include this in the .cpp file
 */

/**
* Creates the memory mapped files
* first: valid address
* second: file_handler (windows only)
*/
std::pair<unsigned char*, void*> create_memory_mapped_files(size_t capacity);

/**
* Destroys the memory mapped files
*/
void destroy_memory_mapped_files(std::pair<unsigned char*, void*> pointer_pair, size_t capacity);

/**
* Get's the page size
*/
size_t page_size() noexcept;
} // namespace detail
} // namespace quill