#include "doctest/doctest.h"

#include "quill/core/LogLevel.h"
#include "quill/sinks/ConsoleSink.h"

TEST_SUITE_BEGIN("ConsoleSink");

using namespace quill;

/***/
TEST_CASE("colours_support_log_level_none")
{
  ConsoleSinkConfig::Colours colours;

  REQUIRE_EQ(colours.log_level_colour(LogLevel::None), std::string_view{});

  colours.assign_colour_to_log_level(LogLevel::None, ConsoleSinkConfig::Colours::white);
  REQUIRE_EQ(colours.log_level_colour(LogLevel::None), ConsoleSinkConfig::Colours::white);
}

TEST_SUITE_END();
