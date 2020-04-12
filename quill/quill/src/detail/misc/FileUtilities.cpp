#include "quill/detail/misc/FileUtilities.h"
#include "quill/Fmt.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/misc/Os.h"
#include <system_error>

namespace quill
{
namespace detail
{
namespace file_utilities
{

/***/
void fwrite_fully(void const* ptr, size_t size, size_t count, FILE* stream)
{
  size_t const written = std::fwrite(ptr, size, count, stream);

  if (QUILL_UNLIKELY(written < count))
  {
    throw std::system_error(errno, std::system_category());
  }
}

/***/
FILE* open(filename_t const& filename, std::string const& mode)
{
  return detail::fopen(filename, mode);
}

/***/
size_t file_size(FILE* file) { return detail::fsize(file); }

/***/
int remove(filename_t const& filename) noexcept { return detail::remove(filename); }

/***/
std::pair<filename_t, filename_t> extract_stem_and_extension(filename_t const& filename) noexcept
{
  // Look for an extensions
  std::size_t const extension_index = filename.rfind('.');

  if (extension_index == filename_t::npos || extension_index == 0 || extension_index == filename.size() - 1)
  {
    // no valid extension found - return whole filename
    return std::make_pair(filename, filename_t{});
  }

  auto const path_index = filename.rfind(path_delimiter);
  if (path_index != filename_t::npos && path_index >= extension_index - 1)
  {
    // treat cases like "/tmp/mc.d/logfile or "/logs/.hiddenlogfile"
    return std::make_pair(filename, filename_t{});
  }

  // filename and extension
  return std::make_pair(filename.substr(0, extension_index), filename.substr(extension_index));
}

/***/
filename_t append_date_to_filename(filename_t const& base_filename,
                                   std::chrono::system_clock::time_point timestamp /* = {} */) noexcept
{
  // Get the time now as tm from user or default to now
  std::chrono::system_clock::time_point const ts_now =
    (timestamp == std::chrono::system_clock::time_point{}) ? std::chrono::system_clock::now() : timestamp;

  time_t time_now = std::chrono::system_clock::to_time_t(ts_now);
  tm now_tm;
  detail::localtime_rs(&time_now, &now_tm);

  // Get base file and extension
  std::pair<filename_t, filename_t> const stem_ext =
    detail::file_utilities::extract_stem_and_extension(base_filename);

  // Construct a filename
  return fmt::format(QUILL_FILENAME_STR("{}_{:04d}-{:02d}-{:02d}{}"), stem_ext.first,
                     now_tm.tm_year + 1900, now_tm.tm_mon + 1, now_tm.tm_mday, stem_ext.second);
}

/***/
filename_t append_index_to_filename(filename_t const& base_filename, uint32_t index) noexcept
{
  if (index == 0u)
  {
    return base_filename;
  }

  // Get base file and extension
  std::pair<filename_t, filename_t> const stem_ext =
    detail::file_utilities::extract_stem_and_extension(base_filename);

  // Construct a filename
  return fmt::format(QUILL_FILENAME_STR("{}.{}{}"), stem_ext.first, index, stem_ext.second);
}

} // namespace file_utilities
} // namespace detail
} // namespace quill