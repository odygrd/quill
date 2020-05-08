#include "quill/detail/misc/Utilities.h"

#include <codecvt> // for codecvt_utf8
#include <locale>  // for wstring_convert

namespace quill
{
namespace detail
{
/***/
std::wstring s2ws(std::string const& str) noexcept
{
  using convert_t = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_t, wchar_t> converter;

  return converter.from_bytes(str);
}

/***/
std::string ws2s(std::wstring const& wstr) noexcept
{
  using convert_t = std::codecvt_utf8<wchar_t>;
  std::wstring_convert<convert_t, wchar_t> converter;

  return converter.to_bytes(wstr);
}

} // namespace detail
} // namespace quill