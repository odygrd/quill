/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/sinks/StreamSink.h"

#ifndef QUILL_MODULE
  #include <array>
  #include <cstdint>
  #include <cstdio>
  #include <cstdlib>
  #include <cstring>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>
#endif

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <io.h>
  #include <windows.h>
#else
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

class ConsoleSinkConfig
{
public:
  enum class ColourMode
  {
    Always,
    Automatic,
    Never
  };

  /**
   * Represents console colours
   */
  class Colours
  {
  public:
    Colours() { apply_default_colours(); }

    /**
     * Sets some default colours for terminal
     */
    void apply_default_colours()
    {
      assign_colour_to_log_level(LogLevel::TraceL3, white);
      assign_colour_to_log_level(LogLevel::TraceL2, white);
      assign_colour_to_log_level(LogLevel::TraceL1, white);
      assign_colour_to_log_level(LogLevel::Debug, cyan);
      assign_colour_to_log_level(LogLevel::Info, green);
      assign_colour_to_log_level(LogLevel::Notice, white_bold);
      assign_colour_to_log_level(LogLevel::Warning, yellow_bold);
      assign_colour_to_log_level(LogLevel::Error, red_bold);
      assign_colour_to_log_level(LogLevel::Critical, bold_on_red);
      assign_colour_to_log_level(LogLevel::Backtrace, magenta);
    }

    /**
     * Sets a custom colour per log level
     * @param log_level the log level
     * @param colour the colour
     */
    void assign_colour_to_log_level(LogLevel log_level, std::string_view colour)
    {
      auto const log_lvl = static_cast<uint32_t>(log_level);
      _log_level_colours[log_lvl] = colour;
    }

    /**
     * The colour for the given log level
     * @param log_level the message log level
     * @return the configured colour for this log level
     */
    QUILL_NODISCARD std::string_view log_level_colour(LogLevel log_level) const noexcept
    {
      auto const log_lvl = static_cast<uint32_t>(log_level);
      return _log_level_colours[log_lvl];
    }

    // Formatting codes
    static constexpr std::string_view reset{"\033[0m"};
    static constexpr std::string_view bold{"\033[1m"};
    static constexpr std::string_view dark{"\033[2m"};
    static constexpr std::string_view underline{"\033[4m"};
    static constexpr std::string_view blink{"\033[5m"};
    static constexpr std::string_view reverse{"\033[7m"};
    static constexpr std::string_view concealed{"\033[8m"};
    static constexpr std::string_view clear_line{"\033[K"};

    // Foreground colors
    static constexpr std::string_view black{"\033[30m"};
    static constexpr std::string_view red{"\033[31m"};
    static constexpr std::string_view green{"\033[32m"};
    static constexpr std::string_view yellow{"\033[33m"};
    static constexpr std::string_view blue{"\033[34m"};
    static constexpr std::string_view magenta{"\033[35m"};
    static constexpr std::string_view cyan{"\033[36m"};
    static constexpr std::string_view white{"\033[37m"};

    /// Background colors
    static constexpr std::string_view on_black{"\033[40m"};
    static constexpr std::string_view on_red{"\033[41m"};
    static constexpr std::string_view on_green{"\033[42m"};
    static constexpr std::string_view on_yellow{"\033[43m"};
    static constexpr std::string_view on_blue{"\033[44m"};
    static constexpr std::string_view on_magenta{"\033[45m"};
    static constexpr std::string_view on_cyan{"\033[46m"};
    static constexpr std::string_view on_white{"\033[47m"};

    /// Bold colors
    static constexpr std::string_view white_bold{"\033[97m\033[1m"};
    static constexpr std::string_view yellow_bold{"\033[33m\033[1m"};
    static constexpr std::string_view red_bold{"\033[31m\033[1m"};
    static constexpr std::string_view bold_on_red{"\033[1m\033[41m"};

  private:
    static_assert(static_cast<size_t>(LogLevel::None) < LogLevelCount,
                  "_log_level_colours must be large enough to be indexed by every LogLevel value");

    // Owned strings: assign_colour_to_log_level() accepts runtime-built colour codes and the
    // backend reads them later, so storing views into caller-owned storage would dangle
    std::array<std::string, LogLevelCount> _log_level_colours; /**< Colours per log level */
  };

  /**
   * @brief Sets custom colours for each log level.
   *
   * This function allows specifying a custom Colours instance, enabling fine-grained control
   * over the colours used for different log levels in the console output.
   *
   * @param colours The Colours instance to use.
   */
  QUILL_ATTRIBUTE_COLD void set_colours(Colours colours) { _colours = std::move(colours); }

  /**
   * @brief Sets the colour mode for console output.
   *
   * This function determines when colours are applied in the console output.
   * Valid options for the colour mode are:
   *   - Always: Colours are always enabled.
   *   - Automatic: Colours are enabled automatically based on the environment
   *     (e.g., terminal support), and are disabled when the NO_COLOR environment variable has a
   *     value, see https://no-color.org.
   *   - Never: Colours are never enabled.
   *
   * The default value is 'Automatic'.
   *
   * @param colour_mode The desired colour mode.
   */
  QUILL_ATTRIBUTE_COLD void set_colour_mode(ColourMode colour_mode) { _colour_mode = colour_mode; }

  /**
   * @brief Sets the output stream for console logging.
   *
   * This function allows selecting the output stream where log messages should be printed.
   * The valid options are:
   *   - "stdout": Logs are printed to the standard output.
   *   - "stderr": Logs are printed to the standard error.
   *
   * The default value is "stdout".
   *
   * @param stream The output stream to use.
   */
  QUILL_ATTRIBUTE_COLD void set_stream(std::string const& stream) { _stream = stream; }

  /**
   * @brief Sets custom pattern formatter options for this sink.
   *
   * By default, the logger's pattern formatter is used to format log messages.
   * This function allows overriding the default formatter with custom options for this specific
   * sink. If a custom formatter is provided, it will be used instead of the logger's formatter.
   *
   * @param options The custom pattern formatter options to use
   */
  QUILL_ATTRIBUTE_COLD void set_override_pattern_formatter_options(std::optional<PatternFormatterOptions> const& options)
  {
    _override_pattern_formatter_options = options;
  }

  /** Getters **/
  QUILL_NODISCARD Colours const& colours() const noexcept { return _colours; }
  QUILL_NODISCARD ColourMode colour_mode() const noexcept { return _colour_mode; }
  QUILL_NODISCARD std::string const& stream() const noexcept { return _stream; }
  QUILL_NODISCARD std::optional<PatternFormatterOptions> const& override_pattern_formatter_options() const noexcept
  {
    return _override_pattern_formatter_options;
  }

private:
  std::optional<PatternFormatterOptions> _override_pattern_formatter_options;
  std::string _stream = "stdout";
  Colours _colours;
  ColourMode _colour_mode{ColourMode::Automatic};
};

/***/
class ConsoleSink : public StreamSink
{
public:
  /**
   * @brief Constructor with custom ConsoleColours
   * config
   */
  QUILL_ATTRIBUTE_COLD explicit ConsoleSink(ConsoleSinkConfig const& config = ConsoleSinkConfig{},
                                            FileEventNotifier file_event_notifier = FileEventNotifier{})
    : StreamSink{_validated_stream(config.stream()), nullptr,
                 config.override_pattern_formatter_options(), std::move(file_event_notifier)},
      _config(config)
  {
    _colours_enabled = _use_colours(_file, _config.colour_mode());
  }

  ~ConsoleSink() override = default;

  /**
   * @brief Write a formatted log message to the stream
   * @param record the log record
   */
  QUILL_ATTRIBUTE_HOT void write_log(LogRecord const& record) override
  {
    if (QUILL_UNLIKELY(!_file))
    {
      return;
    }

    if (_colours_enabled)
    {
      // Apply the user callback to the original statement exactly once and determine newline
      // placement from the transformed result.
      std::string transformed_statement;
      std::string_view statement = record.log_statement;
      if (_file_event_notifier.before_write)
      {
        transformed_statement = _file_event_notifier.before_write(record.log_statement);
        statement = transformed_statement;
      }

      // Reset the colour before the trailing newline, not after it. Terminals with background
      // colour erase fill the remainder of the row with the active background colour when the
      // newline is written, so background colours (e.g. the default Critical bold_on_red)
      // would bleed to the end of the line
      bool const has_trailing_newline = !statement.empty() && (statement.back() == '\n');

      if (has_trailing_newline)
      {
        statement.remove_suffix(1);
      }

      // Build the coloured record and write it with one call: on unbuffered streams each write
      // is a syscall, and separate writes let other output land between colour and reset.
      std::string_view const colour_code = _config.colours().log_level_colour(record.log_level);
      _coloured_statement.clear();
      _coloured_statement.append(colour_code.data(), colour_code.size());
      _coloured_statement.append(statement.data(), statement.size());
      _coloured_statement.append(ConsoleSinkConfig::Colours::reset.data(),
                                 ConsoleSinkConfig::Colours::reset.size());

      if (has_trailing_newline)
      {
        _coloured_statement.push_back('\n');
      }

      // Write directly so before_write is not invoked a second time.
      _write_statement(_coloured_statement);
    }
    else
    {
      // Write record to file
      StreamSink::write_log(record);
    }
  }

  /**
   * @brief Ignores the record. The bytes of a binary record are not text for a terminal, so a
   * logger can have a console sink for its log statements next to a file sink for its binary
   * records. A StreamSink writes them to stdout or stderr.
   */
  QUILL_ATTRIBUTE_HOT void write_binary(BinaryRecord const& /* record */) override {}

protected:
  // protected in case someone wants to derive from this class and create a custom one, e.g. for json logging to stdout
  ConsoleSinkConfig _config;

private:
  /**
   * Checked before the StreamSink is constructed, which takes any other name for the path of a
   * file and creates its directories
   */
  QUILL_NODISCARD static std::string const& _validated_stream(std::string const& stream)
  {
    if (QUILL_UNLIKELY(stream != "stdout" && stream != "stderr"))
    {
      QUILL_THROW(
        QuillError{"Invalid stream name in ConsoleSink constructor, must be 'stdout' or 'stderr'"});
    }

    return stream;
  }

  /** Whether to write colour codes to the stream for the configured colour mode */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _use_colours(FILE* file, ConsoleSinkConfig::ColourMode colour_mode) noexcept
  {
    if (colour_mode == ConsoleSinkConfig::ColourMode::Never)
    {
      return false;
    }

#ifdef _WIN32
    // Enable ANSI colour support on Windows. With ColourMode::Always colours are written even if
    // the console cannot interpret them
    bool const ansi_enabled = _activate_ansi_support(file);
    if (colour_mode == ConsoleSinkConfig::ColourMode::Always)
    {
      return true;
    }

    return ansi_enabled && !_is_no_colour_requested() && _is_terminal_output(file) &&
      _supports_colour_output();
#else
    if (colour_mode == ConsoleSinkConfig::ColourMode::Always)
    {
      return true;
    }

    return !_is_no_colour_requested() && _is_terminal_output(file) && _supports_colour_output();
#endif
  }

  /** @return true if the NO_COLOR environment variable has a value, see https://no-color.org */
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _is_no_colour_requested() noexcept
  {
#if defined(_MSC_VER)
    // Without a buffer the call only reports the size that the value needs, with its terminator
    size_t required_size{0};
    (void)::getenv_s(&required_size, nullptr, 0, "NO_COLOR");
    return required_size > 1;
#else
    char const* const no_colour = std::getenv("NO_COLOR");
    return (no_colour != nullptr) && (no_colour[0] != '\0');
#endif
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _supports_colour_output() noexcept
  {
#ifdef _WIN32
    // On Windows 10 and later, ANSI colors are supported
    return true;
#else
    // Get term from env
    auto* env_p = std::getenv("TERM");

    if (env_p == nullptr)
    {
      return false;
    }

    static constexpr char const* terms[] = {
      "ansi",           "color",      "console",       "cygwin",     "gnome",     "konsole",
      "kterm",          "linux",      "msys",          "putty",      "rxvt",      "screen",
      "vt100",          "xterm",      "tmux",          "terminator", "alacritty", "gnome-terminal",
      "xfce4-terminal", "lxterminal", "mate-terminal", "uxterm",     "eterm",     "tilix",
      "rxvt-unicode",   "kde-konsole"};

    // Loop through each term and check if it's found in env_p
    for (char const* term : terms)
    {
      if (std::strstr(env_p, term) != nullptr)
      {
        // term found
        return true;
      }
    }

    // none of the terms are found
    return false;
#endif
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _is_terminal_output(FILE* file) noexcept
  {
#ifdef _WIN32
    // The stream of a process without a console has no file descriptor
    int const fd = _fileno(file);
    return (fd >= 0) && (_isatty(fd) != 0);
#else
    return ::isatty(fileno(file)) != 0;
#endif
  }

#ifdef _WIN32
  /***/
  QUILL_ATTRIBUTE_COLD static bool _activate_ansi_support(FILE* file) noexcept
  {
    // Try to enable ANSI support for Windows console. The stream of a process without a console
    // has no handle
    HANDLE const out_handle = _get_stream_handle(file);
    if (out_handle == nullptr)
    {
      return false;
    }

    DWORD dw_mode = 0;
    if (!GetConsoleMode(out_handle, &dw_mode))
    {
      return false;
    }

    dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    dw_mode |= ENABLE_PROCESSED_OUTPUT;

    return SetConsoleMode(out_handle, dw_mode) != 0;
  }
#endif

  /**
   * Reused to assemble a coloured record so it is written with one call. Do not replace it with
   * separate writes for the colour code, record and reset: on unbuffered streams such as stderr
   * each write is a syscall (four per line instead of one), and on Windows output from other
   * writers could land between the colour and the reset.
   */
  std::string _coloured_statement;
  bool _colours_enabled{false};
};

QUILL_END_EXPORT

QUILL_END_NAMESPACE
