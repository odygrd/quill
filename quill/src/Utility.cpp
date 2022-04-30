#include "quill/Utility.h"
#include <cstdint> // for uint8_t

// unnamed namespace
namespace
{
template <typename T>
static std::string _to_hex(T* buffer, size_t size) noexcept
{
  static constexpr char const hex_chars[] = "0123456789ABCDEF";

  std::string hex_string;
  hex_string.reserve(3 * size);

  for (size_t i = 0; i < size; ++i)
  {
    // 00001111 mask
    static constexpr uint8_t mask = 0x0Fu;

    // add the first four bits
    hex_string += hex_chars[(buffer[i] >> 4u) & mask];

    // add the remaining bits
    hex_string += hex_chars[buffer[i] & mask];

    if (i != (size - 1))
    {
      // add a space delimiter
      hex_string += ' ';
    }
  }

  return hex_string;
}
} // namespace

namespace quill::utility
{

/***/
std::string to_hex(unsigned char* buffer, size_t size) noexcept { return _to_hex(buffer, size); }

/***/
std::string to_hex(unsigned char const* buffer, size_t size) noexcept
{
  return _to_hex(buffer, size);
}

/***/
std::string to_hex(char* buffer, size_t size) noexcept { return _to_hex(buffer, size); }

/***/
std::string to_hex(char const* buffer, size_t size) noexcept { return _to_hex(buffer, size); }

} // namespace quill::utility