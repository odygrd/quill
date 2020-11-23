#include "quill/detail/serialize/Deserialize.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Utilities.h"
#include <cstdint>
#include <cstring>

namespace quill
{
namespace detail
{
size_t deserialize_argument(unsigned char const*& read_buffer,
                            fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store,
                            std::string const& type_descriptor)
{
  // size of argument we deserialized
  size_t read_size{0};

  if (std::strcmp(type_descriptor.data(), "B") == 0)
  {
    using type_t = bool;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "IS") == 0)
  {
    using type_t = short;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "I") == 0)
  {
    using type_t = int;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "IL") == 0)
  {
    using type_t = long;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "ILL") == 0)
  {
    using type_t = long long;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "UIS") == 0)
  {
    using type_t = unsigned short;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "UI") == 0)
  {
    using type_t = unsigned int;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "UIL") == 0)
  {
    using type_t = unsigned long;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "UILL") == 0)
  {
    using type_t = unsigned long long;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "D") == 0)
  {
    using type_t = double;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "LD") == 0)
  {
    using type_t = long double;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "F") == 0)
  {
    using type_t = float;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "C") == 0)
  {
    using type_t = char;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "UC") == 0)
  {
    using type_t = unsigned char;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "CS") == 0)
  {
    using type_t = signed char;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "P") == 0)
  {
    using type_t = void*;
    fmt_store.push_back(*(reinterpret_cast<type_t const*>(read_buffer)));

    read_buffer += sizeof(type_t);
    read_size = sizeof(type_t);
  }
  else if (std::strcmp(type_descriptor.data(), "SC") == 0 ||
           std::strcmp(type_descriptor.data(), "S") == 0)
  {
    auto str = reinterpret_cast<char const*>(read_buffer);
    fmt_store.push_back(str);

    // Add the null termination char
    size_t const len = std::strlen(str) + 1;
    read_buffer += len;
    read_size = len;
  }
  else
  {
    QUILL_THROW(QuillError{"Unknown type descriptor. [" + type_descriptor + "] Can not de-serialize."});
  }

  return read_size;
}
} // namespace detail
} // namespace quill