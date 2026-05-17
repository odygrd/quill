#include "doctest/doctest.h"

#include "quill/sinks/ConsoleSink.h"

using namespace quill;

/***/
TEST_CASE("console_sink_invalid_stream_throws")
{
  ConsoleSinkConfig config;
  config.set_stream("invalid_stream");
  config.set_colour_mode(ConsoleSinkConfig::ColourMode::Never);

  REQUIRE_THROWS_AS(ConsoleSink{config}, QuillError);
}
