#include "doctest/doctest.h"

#include "quill/core/LogLevel.h"
#include "quill/sinks/ConsoleSink.h"

TEST_SUITE_BEGIN("ConsoleSink");

using namespace quill;

namespace
{
class TestConsoleSink : public ConsoleSink
{
public:
  TestConsoleSink(ConsoleSinkConfig const& config, FileEventNotifier notifier)
    : ConsoleSink(config, std::move(notifier))
  {
#if defined(_WIN32)
    if (::tmpfile_s(&_file) != 0)
    {
      _file = nullptr;
    }
#else
    _file = std::tmpfile();
#endif
  }

  ~TestConsoleSink() override
  {
    if (_file)
    {
      std::fclose(_file);
    }
  }

  bool has_file() const noexcept { return _file != nullptr; }

  std::string contents()
  {
    if ((std::fflush(_file) != 0) || (std::fseek(_file, 0, SEEK_END) != 0))
    {
      return {};
    }

    long const file_size = std::ftell(_file);
    if (file_size < 0)
    {
      return {};
    }

    std::rewind(_file);

    std::string result(static_cast<size_t>(file_size), '\0');
    if (!result.empty())
    {
      size_t const bytes_read = std::fread(result.data(), sizeof(char), result.size(), _file);
      result.resize(bytes_read);
    }
    return result;
  }
};
} // namespace

/***/
TEST_CASE("colours_support_log_level_none")
{
  ConsoleSinkConfig::Colours colours;

  REQUIRE_EQ(colours.log_level_colour(LogLevel::None), std::string_view{});

  colours.assign_colour_to_log_level(LogLevel::None, ConsoleSinkConfig::Colours::white);
  REQUIRE_EQ(colours.log_level_colour(LogLevel::None), ConsoleSinkConfig::Colours::white);
}

/***/
TEST_CASE("colours_own_assigned_colour_strings")
{
  // Regression: assigned colours were stored as string_view into caller-owned storage, so a
  // runtime-built colour string dangled by the time the backend read it. The colour must be
  // copied and remain valid after the source string is destroyed
  ConsoleSinkConfig::Colours colours;

  {
    std::string runtime_colour{"\033[38;5;"};
    runtime_colour += std::to_string(208);
    runtime_colour += 'm';
    colours.assign_colour_to_log_level(LogLevel::Info, runtime_colour);
  }

  REQUIRE_EQ(colours.log_level_colour(LogLevel::Info), std::string_view{"\033[38;5;208m"});
}

/***/
TEST_CASE("coloured_console_applies_before_write_before_newline_handling")
{
  ConsoleSinkConfig config;
  config.set_colour_mode(ConsoleSinkConfig::ColourMode::Always);

  std::string callback_input;
  FileEventNotifier notifier;
  notifier.before_write = [&callback_input](std::string_view statement)
  {
    callback_input.assign(statement.data(), statement.size());
    return std::string{"changed\n"};
  };

  TestConsoleSink sink{config, std::move(notifier)};
  REQUIRE(sink.has_file());

  sink.write_log(nullptr, 0, std::string_view{}, std::string_view{}, std::string{},
                 std::string_view{}, LogLevel::Info, "INFO", "I", nullptr, "message", "original\n");

  REQUIRE_EQ(callback_input, "original\n");

  std::string expected{ConsoleSinkConfig::Colours::green};
  expected += "changed";
  expected += ConsoleSinkConfig::Colours::reset;
  expected += '\n';
  REQUIRE_EQ(sink.contents(), expected);
}

TEST_SUITE_END();
