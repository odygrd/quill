/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/sinks/StreamSink.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
  #include <cstdlib>
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

/** Forward Declaration **/
class MacroMetadata;

/***/
class ConsoleSink : public StreamSink
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
    Colours()
    {
      // by default _using_colours is false
      _log_level_colours.fill(white);
    }

    ~Colours() = default;

    /**
     * Sets some default colours for terminal
     */
    void apply_default_colours() noexcept
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
    void assign_colour_to_log_level(LogLevel log_level, std::string_view colour) noexcept
    {
      auto const log_lvl = static_cast<uint32_t>(log_level);
      _log_level_colours[log_lvl] = colour;
      _colours_enabled = true;
    }

    /**
     * @return true if we are in terminal and have also enabled colours
     */
    QUILL_NODISCARD bool colours_enabled() const noexcept
    {
      return _colour_output_supported && _colours_enabled;
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
    friend class ConsoleSink;

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

      static constexpr const char* terms[] = {
        "ansi",          "color",     "console",        "cygwin",         "gnome",
        "konsole",       "kterm",     "linux",          "msys",           "putty",
        "rxvt",          "screen",    "vt100",          "xterm",          "tmux",
        "terminator",    "alacritty", "gnome-terminal", "xfce4-terminal", "lxterminal",
        "mate-terminal", "uxterm",    "eterm",          "tilix",          "rxvt-unicode",
        "kde-konsole"};

      // Loop through each term and check if it's found in env_p
      for (const char* term : terms)
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
      return _isatty(_fileno(file)) != 0;
#else
      return ::isatty(fileno(file)) != 0;
#endif
    }

#ifdef _WIN32
    /***/
    QUILL_ATTRIBUTE_COLD void _activate_ansi_support(FILE* file) const
    {
      if (!_colour_output_supported)
      {
        return;
      }

      // Try to enable ANSI support for Windows console
      auto const out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file)));
      if (out_handle == INVALID_HANDLE_VALUE)
      {
        return;
      }

      DWORD dw_mode = 0;
      if (!GetConsoleMode(out_handle, &dw_mode))
      {
        return;
      }

      dw_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      dw_mode |= ENABLE_PROCESSED_OUTPUT;

      SetConsoleMode(out_handle, dw_mode);
    }
#endif

    /***/
    void _configure_colour_support(FILE* file, ColourMode colour_mode) noexcept
    {
      if (colour_mode == ColourMode::Always)
      {
        _colour_output_supported = true;
      }
      else if (colour_mode == ColourMode::Automatic)
      {
        _colour_output_supported = _is_terminal_output(file) && _supports_colour_output();
      }
      else
      {
        _colour_output_supported = false;
      }

#ifdef _WIN32
      // Enable ANSI color support on Windows
      _activate_ansi_support(file);
#endif
    }

  private:
    std::array<std::string_view, 10> _log_level_colours; /**< Colours per log level */
    bool _colours_enabled{false};
    bool _colour_output_supported{false};
  };

  /**
   * @brief Constructor with custom ConsoleColours
   * @param colours console colours instance
   * @param colour_mode Determines when console colours are enabled.
   *                    - Always: Colours are always enabled.
   *                    - Automatic: Colours are enabled automatically based on the environment (e.g., terminal support).
   *                    - Never: Colours are never enabled.
   * @param stream stream name can only be "stdout" or "stderr"
   */
  explicit ConsoleSink(Colours const& colours, ColourMode colour_mode = ColourMode::Automatic,
                       std::string const& stream = "stdout")
    : StreamSink{stream, nullptr}, _colours(colours)
  {
    assert((stream == "stdout") || (stream == "stderr"));

    // In this ctor we take a full copy of colours and in our instance we modify it
    _colours._configure_colour_support(_file, colour_mode);
  }

  /**
   * @brief Constructor with default ConsoleColours
   * @param colour_mode Determines when console colours are enabled.
   *                    - Always: Colours are always enabled.
   *                    - Automatic: Colours are enabled automatically based on the environment (e.g., terminal support).
   *                    - Never: Colours are never enabled.
   * @param stream stream name can only be "stdout" or "stderr"
   */
  explicit ConsoleSink(ColourMode colour_mode = ColourMode::Automatic,
                       std::string const& stream = "stdout")
    : StreamSink{stream, nullptr}
  {
    assert((stream == "stdout") || (stream == "stderr"));

    _colours._configure_colour_support(_file, colour_mode);

    if (colour_mode != ColourMode::Never)
    {
      _colours.apply_default_colours();
    }
  }

  ~ConsoleSink() override = default;

  /**
   * @brief Write a formatted log message to the stream
   * @param log_metadata log metadata
   * @param log_timestamp log timestamp
   * @param thread_id thread id
   * @param thread_name thread name
   * @param process_id Process Id
   * @param logger_name logger name
   * @param log_level Log level of the message.
   * @param log_level_description Description of the log level.
   * @param log_level_short_code Short code representing the log level.
   * @param named_args vector of key-value pairs of named args
   * @param log_message log message
   * @param log_statement log statement
   */
  QUILL_ATTRIBUTE_HOT void write_log(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                     std::string_view thread_id, std::string_view thread_name,
                                     std::string const& process_id, std::string_view logger_name,
                                     LogLevel log_level, std::string_view log_level_description,
                                     std::string_view log_level_short_code,
                                     std::vector<std::pair<std::string, std::string>> const* named_args,
                                     std::string_view log_message, std::string_view log_statement) override
  {
    if (_colours.colours_enabled())
    {
      // Write colour code
      std::string_view const colour_code = _colours.log_level_colour(log_level);
      safe_fwrite(colour_code.data(), sizeof(char), colour_code.size(), _file);

      // Write record to file
      StreamSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                            logger_name, log_level, log_level_description, log_level_short_code,
                            named_args, log_message, log_statement);

      // Reset colour code
      safe_fwrite(Colours::reset.data(), sizeof(char), Colours::reset.size(), _file);
    }
    else
    {
      // Write record to file
      StreamSink::write_log(log_metadata, log_timestamp, thread_id, thread_name, process_id,
                            logger_name, log_level, log_level_description, log_level_short_code,
                            named_args, log_message, log_statement);
    }
  }

protected:
  // protected in case someone wants to derive from this class and create a custom one, e.g. for json logging to stdout
  Colours _colours;
};

QUILL_END_NAMESPACE