#include "doctest/doctest.h"

#include "quill/core/LogLevel.h"
#include "quill/sinks/ConsoleSink.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

#if !defined(_WIN32)
  #include <fcntl.h>
  #include <poll.h>
  #include <unistd.h>
#endif

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

  sink.write_log(LogRecord{nullptr,
                           0,
                           std::string_view{},
                           std::string_view{},
                           std::string{},
                           std::string_view{},
                           LogLevel::Info,
                           "INFO",
                           "I",
                           nullptr,
                           "message",
                           "original\n",
                           {}});

  REQUIRE_EQ(callback_input, "original\n");

  std::string expected{ConsoleSinkConfig::Colours::green};
  expected += "changed";
  expected += ConsoleSinkConfig::Colours::reset;
  expected += '\n';
  REQUIRE_EQ(sink.contents(), expected);
}

/***/
TEST_CASE("automatic_colour_mode_follows_the_no_color_environment_variable")
{
#if defined(_WIN32)
  return;
#else
  // A pseudo terminal takes the place of stdout, so that the automatic mode finds a terminal
  int const terminal_fd = ::posix_openpt(O_RDWR | O_NOCTTY);

  if ((terminal_fd < 0) || (::grantpt(terminal_fd) != 0) || (::unlockpt(terminal_fd) != 0))
  {
    // No pseudo terminals here, e.g. in a container without /dev/pts
    if (terminal_fd >= 0)
    {
      ::close(terminal_fd);
    }

    return;
  }

  char const* const terminal_name = ::ptsname(terminal_fd);
  int const stdout_terminal_fd = terminal_name ? ::open(terminal_name, O_RDWR | O_NOCTTY) : -1;

  // A read of what the sink wrote must not wait when there is nothing more to read
  if ((stdout_terminal_fd < 0) || (::fcntl(terminal_fd, F_SETFL, O_NONBLOCK) != 0))
  {
    if (stdout_terminal_fd >= 0)
    {
      ::close(stdout_terminal_fd);
    }

    ::close(terminal_fd);
    return;
  }

  char const* const previous_term = std::getenv("TERM");
  std::string const term_to_restore{previous_term ? previous_term : ""};
  char const* const previous_no_color = std::getenv("NO_COLOR");
  std::string const no_color_to_restore{previous_no_color ? previous_no_color : ""};

  std::fflush(stdout);
  int const saved_stdout_fd = ::dup(STDOUT_FILENO);
  ::dup2(stdout_terminal_fd, STDOUT_FILENO);
  ::setenv("TERM", "xterm", 1);

  // Whether a sink that is created with this value of NO_COLOR writes colour codes
  auto const writes_colours = [terminal_fd](char const* no_color)
  {
    if (no_color)
    {
      ::setenv("NO_COLOR", no_color, 1);
    }
    else
    {
      ::unsetenv("NO_COLOR");
    }

    ConsoleSink sink{ConsoleSinkConfig{}};
    sink.write_log(LogRecord{
      nullptr, 0, {}, {}, {}, {}, LogLevel::Info, "INFO", "I", nullptr, "message", "message\n", {}});
    sink.flush_sink(SinkFlushReason::Explicit);

    // The terminal passes on what is written to it a moment later. A mark after what the sink
    // wrote tells where that ends, so the read waits for the mark instead of taking what is there
    static constexpr char end_mark{'\x1f'};
    ssize_t const mark_size = ::write(STDOUT_FILENO, &end_mark, 1);

    std::string written;
    pollfd terminal{terminal_fd, POLLIN, 0};

    while ((mark_size == 1) && (written.find(end_mark) == std::string::npos) &&
           (::poll(&terminal, 1, 10'000) > 0))
    {
      char chunk[128];
      ssize_t const size = ::read(terminal_fd, chunk, sizeof(chunk));

      if (size <= 0)
      {
        break;
      }

      written.append(chunk, static_cast<size_t>(size));
    }

    return written.find("\033[") != std::string::npos;
  };

  bool const without_variable = writes_colours(nullptr);
  bool const with_empty_variable = writes_colours("");
  bool const with_variable = writes_colours("1");

  // Back to the stdout of the test, which reports to it
  std::fflush(stdout);
  ::dup2(saved_stdout_fd, STDOUT_FILENO);
  ::close(saved_stdout_fd);
  ::close(stdout_terminal_fd);
  ::close(terminal_fd);

  if (previous_term)
  {
    ::setenv("TERM", term_to_restore.c_str(), 1);
  }
  else
  {
    ::unsetenv("TERM");
  }

  if (previous_no_color)
  {
    ::setenv("NO_COLOR", no_color_to_restore.c_str(), 1);
  }
  else
  {
    ::unsetenv("NO_COLOR");
  }

  REQUIRE(without_variable);
  REQUIRE(with_empty_variable);
  REQUIRE_FALSE(with_variable);
#endif
}

/***/
TEST_CASE("console_sink_ignores_binary_records")
{
  ConsoleSinkConfig config;
  config.set_colour_mode(ConsoleSinkConfig::ColourMode::Never);

  TestConsoleSink sink{config, FileEventNotifier{}};
  REQUIRE(sink.has_file());

  // A logger can have a console sink for its log statements next to a file sink for its binary
  // records, whose bytes are not text for a terminal
  std::string const bytes{"\x1b[2J\x00\x01", 6};
  sink.write_binary(BinaryRecord{nullptr, 0, std::string_view{}, std::string_view{},
                                 std::string_view{}, std::string_view{},
                                 reinterpret_cast<std::byte const*>(bytes.data()), bytes.size()});

  sink.write_log(LogRecord{nullptr,
                           0,
                           std::string_view{},
                           std::string_view{},
                           std::string{},
                           std::string_view{},
                           LogLevel::Info,
                           "INFO",
                           "I",
                           nullptr,
                           "message",
                           "statement\n",
                           {}});

  REQUIRE_EQ(sink.contents(), "statement\n");
}

TEST_SUITE_END();
