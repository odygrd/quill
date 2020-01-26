#include "quill/sinks/StdoutSink.h"

#include <cstdio>

#include "quill/detail/CommonUtilities.h"

namespace quill
{

/***/
StdoutSink* StdoutSink::clone() const { return new StdoutSink{*this}; }

/***/
void StdoutSink::log(fmt::memory_buffer const& formatted_line)
{
  detail::fwrite_fully(formatted_line.data(), sizeof(char), formatted_line.size(), stdout);
}

/***/
void StdoutSink::flush() { fflush(stdout); }

} // namespace quill