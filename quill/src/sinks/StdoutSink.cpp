#include "quill/sinks/StdoutSink.h"

#include "quill/detail/Utilities.h"

namespace quill::detail
{
/***/
void StdoutSink::log(fmt::memory_buffer const& formatted_line)
{
  fwrite_fully(formatted_line.data(), sizeof(char), formatted_line.size(), stdout);
}

} // namespace quill::detail