#include "quill/detail/serialize/Deserialize.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Utilities.h"
#include <cstdint>
#include <cstring>
#include <string>

namespace quill
{
namespace detail
{

namespace
{
template <typename TValue>
QUILL_NODISCARD size_t get_value(unsigned char const*& read_buffer,
                                 fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store)
{
  using value_t = TValue;

  value_t value;
  std::memcpy(&value, read_buffer, sizeof(value_t));
  fmt_store.push_back(value);

  read_buffer += sizeof(value_t);
  return sizeof(value_t);
}

template <>
QUILL_NODISCARD size_t get_value<char const*>(unsigned char const*& read_buffer,
                                              fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store)
{
  // string in buffer is null terminated
  fmt_store.push_back(reinterpret_cast<char const*>(read_buffer));

  size_t len = std::strlen(reinterpret_cast<char const*>(read_buffer)) + 1;
  read_buffer += len;
  return len;
}

} // namespace
size_t deserialize_argument(unsigned char const*& read_buffer,
                            fmt::dynamic_format_arg_store<fmt::format_context>& fmt_store,
                            TypeDescriptor type_descriptor)
{
  // size of argument we deserialized
  size_t read_size;

  switch (type_descriptor)
  {
  case TypeDescriptor::Bool:
    read_size = get_value<bool>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Short:
    read_size = get_value<short>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Int:
    read_size = get_value<int>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Long:
    read_size = get_value<long>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::LongLong:
    read_size = get_value<long long>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::UnsignedShort:
    read_size = get_value<unsigned short>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::UnsignedInt:
    read_size = get_value<unsigned int>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::UnsignedLong:
    read_size = get_value<unsigned long>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::UnsignedLongLong:
    read_size = get_value<unsigned long long>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Double:
    read_size = get_value<double>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::LongDouble:
    read_size = get_value<long double>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Float:
    read_size = get_value<float>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::Char:
    read_size = get_value<char>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::UnsignedChar:
    read_size = get_value<unsigned char>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::SignedChar:
    read_size = get_value<signed char>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::VoidPtr:
    read_size = get_value<void*>(read_buffer, fmt_store);
    break;
  case TypeDescriptor::String:
    read_size = get_value<char const*>(read_buffer, fmt_store);
    break;
  default:
    QUILL_THROW(QuillError{"Unknown type descriptor. [" +
                           std::string{static_cast<char>(type_descriptor)} + "] Can not de-serialize."});
  }

  return read_size;
}
} // namespace detail
} // namespace quill