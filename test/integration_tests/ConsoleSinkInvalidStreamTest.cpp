#include "doctest/doctest.h"

#include "quill/core/Filesystem.h"
#include "quill/sinks/ConsoleSink.h"

#include <string>
#include <system_error>

using namespace quill;

/***/
TEST_CASE("console_sink_invalid_stream_throws")
{
  ConsoleSinkConfig config;
  config.set_stream("invalid_stream");
  config.set_colour_mode(ConsoleSinkConfig::ColourMode::Never);

#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(ConsoleSink{config}, QuillError);

  // A name that is not a console stream is not taken for the path of a file either, whose
  // directory would be created
  fs::path const directory{"console_sink_invalid_stream_directory"};
  std::error_code ec;
  fs::remove_all(directory, ec);

  config.set_stream((directory / "stream").string());
  REQUIRE_THROWS_AS(ConsoleSink{config}, QuillError);
  REQUIRE_FALSE(fs::exists(directory));
#endif
}
