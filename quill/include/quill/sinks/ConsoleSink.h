/**
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

namespace quill
{

/** Forward Declaration **/
class MacroMetadata;

#if defined(_WIN32)
/**
 * Represents console colours
 */
class ConsoleColours
{
private:
  // define our own to avoid including windows.h in the header..
  using WORD = unsigned short;

public:
  ConsoleColours() { _colours.fill(white); }
  ~ConsoleColours() = default;

  /**
   * Sets some default colours for terminal
   */
  void set_default_colours() noexcept
  {
    set_colour(LogLevel::TraceL3, white);
    set_colour(LogLevel::TraceL2, white);
    set_colour(LogLevel::TraceL1, white);
    set_colour(LogLevel::Debug, cyan);
    set_colour(LogLevel::Info, green);
    set_colour(LogLevel::Warning, yellow | bold);
    set_colour(LogLevel::Error, red | bold);
    set_colour(LogLevel::Critical, on_red | bold | white); // white bold on red background
    set_colour(LogLevel::Backtrace, magenta);
  }

  /**
   * Sets a custom colour per log level
   * @param log_level the log level
   * @param colour the colour
   */
  void set_colour(LogLevel log_level, WORD colour) noexcept
  {
    auto const log_lvl = static_cast<uint32_t>(log_level);
    _colours[log_lvl] = colour;
    _using_colours = true;
  }

  /**
   * @return true if we are in terminal and have also enabled colours
   */
  QUILL_NODISCARD bool can_use_colours() const noexcept
  {
    return _can_use_colours && _using_colours;
  }

  /**
   * @return true if we have setup colours
   */
  QUILL_NODISCARD bool using_colours() const noexcept { return _using_colours; }

  /**
   * The colour for the given log level
   * @param log_level the message log level
   * @return the configured colour for this log level
   */
  QUILL_NODISCARD WORD colour_code(LogLevel log_level) const noexcept
  {
    auto const log_lvl = static_cast<uint32_t>(log_level);
    return _colours[log_lvl];
  }

  static inline WORD const bold = FOREGROUND_INTENSITY;

  static inline WORD const black = 0;
  static inline WORD const red = FOREGROUND_RED;
  static inline WORD const green = FOREGROUND_GREEN;
  static inline WORD const yellow = FOREGROUND_RED | FOREGROUND_GREEN;
  static inline WORD const blue = FOREGROUND_BLUE;
  static inline WORD const magenta = FOREGROUND_RED | FOREGROUND_BLUE;
  static inline WORD const cyan = FOREGROUND_GREEN | FOREGROUND_BLUE;
  static inline WORD const white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

  static inline WORD const on_red = BACKGROUND_RED;
  static inline WORD const on_green = BACKGROUND_GREEN;
  static inline WORD const on_yellow = BACKGROUND_RED | BACKGROUND_GREEN;
  static inline WORD const on_blue = BACKGROUND_BLUE;
  static inline WORD const on_magenta = BACKGROUND_RED | BACKGROUND_BLUE;
  static inline WORD const on_cyan = BACKGROUND_GREEN | BACKGROUND_BLUE;
  static inline WORD const on_white = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_BLUE;

private:
  friend class ConsoleSink;

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _is_in_terminal(FILE* file) noexcept
  {
    bool const is_atty = _isatty(_fileno(file)) != 0;

    // ::GetConsoleMode() should return 0 if file is redirected or does not point to the actual console
    DWORD console_mode;
    bool const is_console =
      GetConsoleMode(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))), &console_mode) != 0;

    return is_atty && is_console;
  }

  void _set_can_use_colours(FILE* file) noexcept { _can_use_colours = _is_in_terminal(file); }

private:
  std::array<WORD, 10> _colours = {0}; /**< Colours per log level */
  bool _using_colours{false};
  bool _can_use_colours{false};
};
#else
/**
 * Represents console colours
 */
class ConsoleColours
{
public:
  ConsoleColours() { _colours.fill(white); }

  ~ConsoleColours() = default;

  /**
   * Sets some default colours for terminal
   */
  void set_default_colours() noexcept
  {
    set_colour(LogLevel::TraceL3, white);
    set_colour(LogLevel::TraceL2, white);
    set_colour(LogLevel::TraceL1, white);
    set_colour(LogLevel::Debug, cyan);
    set_colour(LogLevel::Info, green);
    set_colour(LogLevel::Warning, yellow_bold);
    set_colour(LogLevel::Error, red_bold);
    set_colour(LogLevel::Critical, bold_on_red);
    set_colour(LogLevel::Backtrace, magenta);
  }

  /**
   * Sets a custom colour per log level
   * @param log_level the log level
   * @param colour the colour
   */

  void set_colour(LogLevel log_level, std::string const& colour) noexcept
  {
    auto const log_lvl = static_cast<uint32_t>(log_level);
    _colours[log_lvl] = colour;
    _using_colours = true;
  }

  /**
   * @return true if we are in terminal and have also enabled colours
   */
  QUILL_NODISCARD bool can_use_colours() const noexcept
  {
    return _can_use_colours && _using_colours;
  }

  /**
   * @return true if we have setup colours
   */
  QUILL_NODISCARD bool using_colours() const noexcept { return _using_colours; }

  /**
   * The colour for the given log level
   * @param log_level the message log level
   * @return the configured colour for this log level
   */
  QUILL_NODISCARD std::string const& colour_code(LogLevel log_level) const noexcept
  {
    auto const log_lvl = static_cast<uint32_t>(log_level);
    return _colours[log_lvl];
  }

  // Formatting codes
  static inline std::string const reset{"\033[0m"};
  static inline std::string const bold{"\033[1m"};
  static inline std::string const dark{"\033[2m"};
  static inline std::string const underline{"\033[4m"};
  static inline std::string const blink{"\033[5m"};
  static inline std::string const reverse{"\033[7m"};
  static inline std::string const concealed{"\033[8m"};
  static inline std::string const clear_line{"\033[K"};

  // Foreground colors
  static inline std::string const black{"\033[30m"};
  static inline std::string const red{"\033[31m"};
  static inline std::string const green{"\033[32m"};
  static inline std::string const yellow{"\033[33m"};
  static inline std::string const blue{"\033[34m"};
  static inline std::string const magenta{"\033[35m"};
  static inline std::string const cyan{"\033[36m"};
  static inline std::string const white{"\033[37m"};

  /// Background colors
  static inline std::string const on_black{"\033[40m"};
  static inline std::string const on_red{"\033[41m"};
  static inline std::string const on_green{"\033[42m"};
  static inline std::string const on_yellow{"\033[43m"};
  static inline std::string const on_blue{"\033[44m"};
  static inline std::string const on_magenta{"\033[45m"};
  static inline std::string const on_cyan{"\033[46m"};
  static inline std::string const on_white{"\033[47m"};

  /// Bold colors
  static inline std::string const yellow_bold{"\033[33m\033[1m"};
  static inline std::string const red_bold{"\033[31m\033[1m"};
  static inline std::string const bold_on_red{"\033[1m\033[41m"};

private:
  friend class ConsoleSink;

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _is_colour_terminal() noexcept
  {
    // Get term from env
    auto* env_p = std::getenv("TERM");

    if (env_p == nullptr)
    {
      return false;
    }

    static constexpr const char* terms[] = {"ansi",    "color",  "console", "cygwin", "gnome",
                                            "konsole", "kterm",  "linux",   "msys",   "putty",
                                            "rxvt",    "screen", "vt100",   "xterm",  "tmux"};

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
  }

  /***/
  QUILL_NODISCARD QUILL_ATTRIBUTE_COLD static bool _is_in_terminal(FILE* file) noexcept
  {
    return ::isatty(fileno(file)) != 0;
  }

  void _set_can_use_colours(FILE* file) noexcept
  {
    _can_use_colours = _is_in_terminal(file) && _is_colour_terminal();
  }

private:
  std::array<std::string, 10> _colours; /**< Colours per log level */
  bool _using_colours{false};
  bool _can_use_colours{false};
};
#endif

/***/
class ConsoleSink : public StreamSink
{
public:
  /**
   * @brief Constructor
   * @param console_colours console colours instance
   * @param stream stream name can only be "stdout" or "stderr"
   */
  explicit ConsoleSink(ConsoleColours const& console_colours, std::string const& stream = "stdout")
    : StreamSink{stream, nullptr}, _console_colours(console_colours)
  {
    assert((stream == "stdout") || (stream == "stderr"));

    // In this ctor we take a full copy of console_colours and in our instance we modify it
    _console_colours._set_can_use_colours(_file);
  }

  /**
   * @brief Constructor
   * @param enable_colours enable or disable console colours
   * @param stream stream name can only be "stdout" or "stderr"
   */
  explicit ConsoleSink(bool enable_colours = true, std::string const& stream = "stdout")
    : StreamSink{stream, nullptr}
  {
    assert((stream == "stdout") || (stream == "stderr"));

    if (enable_colours)
    {
      _console_colours._set_can_use_colours(_file);
      _console_colours.set_default_colours();
    }
  }

  ~ConsoleSink() override = default;

  /**
   * @brief Write a formatted log message to the stream
   * @param log_metadata log metadata
   * @param log_timestamp log timestamp
   * @param thread_id thread id
   * @param thread_name thread name
   * @param logger_name logger name
   * @param log_level log level
   * @param named_args vector of key-value pairs of named args
   * @param log_message log message
   */
  QUILL_ATTRIBUTE_HOT void write_log_message(MacroMetadata const* log_metadata, uint64_t log_timestamp,
                                             std::string_view thread_id, std::string_view thread_name,
                                             std::string_view logger_name, LogLevel log_level,
                                             std::vector<std::pair<std::string, std::string>> const* named_args,
                                             std::string_view log_message) override
  {
#if defined(_WIN32)
    if (_console_colours.using_colours())
    {
      WORD const colour_code = _console_colours.colour_code(log_level);

      // Set foreground colour and store the original attributes
      WORD const orig_attribs = _set_foreground_colour(colour_code);

      auto out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(_file)));

      // Write to console
      bool const write_to_console = WriteConsoleA(
        out_handle, log_message.data(), static_cast<DWORD>(log_message.size()), nullptr, nullptr);

      if (QUILL_UNLIKELY(!write_to_console))
      {
        auto const error = std::error_code(GetLastError(), std::system_category());
        QUILL_THROW(QuillError{std::string{"WriteConsoleA failed. error: "} + error.message() +
                               std::string{" errno: "} + std::to_string(error.value())});
      }

      // reset to orig colors
      bool const set_text_attr = SetConsoleTextAttribute(out_handle, orig_attribs);
      if (QUILL_UNLIKELY(!set_text_attr))
      {
        auto const error = std::error_code(GetLastError(), std::system_category());
        QUILL_THROW(QuillError{std::string{"SetConsoleTextAttribute failed. error: "} + error.message() +
                               std::string{" errno: "} + std::to_string(error.value())});
      }
    }
    else
    {
      // Write record to file
      StreamSink::write_log_message(log_metadata, log_timestamp, thread_id, thread_name,
                                    logger_name, log_level, named_args, log_message);
    }
#else
    if (_console_colours.can_use_colours())
    {
      // Write colour code
      std::string const& colour_code = _console_colours.colour_code(log_level);
      safe_fwrite(colour_code.data(), sizeof(char), colour_code.size(), _file);
    }

    // Write record to file
    StreamSink::write_log_message(log_metadata, log_timestamp, thread_id, thread_name, logger_name,
                                  log_level, named_args, log_message);

    if (_console_colours.can_use_colours())
    {
      safe_fwrite(ConsoleColours::reset.data(), sizeof(char), ConsoleColours::reset.size(), _file);
    }
#endif
  }

  /**
   * Used internally to enable the console colours on "stdout" sink
   */
  QUILL_ATTRIBUTE_COLD void enable_console_colours() noexcept
  {
    _console_colours.set_default_colours();
  }

private:
#if defined(_WIN32)
  QUILL_NODISCARD ConsoleColours::WORD _set_foreground_colour(ConsoleColours::WORD attributes)
  {
    CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
    auto const out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(_file)));

    bool const screen_buffer_info = GetConsoleScreenBufferInfo(out_handle, &orig_buffer_info);
    if (QUILL_UNLIKELY(!screen_buffer_info))
    {
      auto const error = std::error_code(GetLastError(), std::system_category());
      QUILL_THROW(QuillError{std::string{"GetConsoleScreenBufferInfo failed. error: "} +
                             error.message() + std::string{" errno: "} + std::to_string(error.value())});
    }

    WORD back_color = orig_buffer_info.wAttributes;

    // retrieve the current background color
    back_color &=
      static_cast<WORD>(~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY));

    // keep the background color unchanged
    bool const console_text_attr = SetConsoleTextAttribute(out_handle, attributes | back_color);
    if (QUILL_UNLIKELY(!console_text_attr))
    {
      auto const error = std::error_code(GetLastError(), std::system_category());
      QUILL_THROW(QuillError{std::string{"SetConsoleTextAttribute failed. error: "} + error.message() +
                             std::string{" errno: "} + std::to_string(error.value())});
    }

    return orig_buffer_info.wAttributes; // return orig attribs
  }
#endif

protected:
  // protected in case someone wants to derive from this class and create a custom one, e.g. for json logging to stdout
  ConsoleColours _console_colours;
};
} // namespace quill
