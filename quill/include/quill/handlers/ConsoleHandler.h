/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/common/Attributes.h"
#include "quill/common/FileUtilities.h" // for fwrite_fully
#include "quill/common/LogLevel.h"
#include "quill/common/Os.h"
#include "quill/handlers/StreamHandler.h" // for StreamHandler

#include <array>
#include <string> // for string

#include <cassert>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif

  #include <windows.h>

  #include <io.h>
  #include <wincon.h>
#endif

namespace quill
{

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
    using log_lvl_t = std::underlying_type_t<LogLevel>;
    auto const log_lvl = static_cast<log_lvl_t>(log_level);
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
    using log_lvl_t = std::underlying_type_t<LogLevel>;
    auto const log_lvl = static_cast<log_lvl_t>(log_level);
    return _colours[log_lvl];
  }

  QUILL_API static inline WORD const bold = FOREGROUND_INTENSITY;

  QUILL_API static inline WORD const black = 0;
  QUILL_API static inline WORD const red = FOREGROUND_RED;
  QUILL_API static inline WORD const green = FOREGROUND_GREEN;
  QUILL_API static inline WORD const yellow = FOREGROUND_RED | FOREGROUND_GREEN;
  QUILL_API static inline WORD const blue = FOREGROUND_BLUE;
  QUILL_API static inline WORD const magenta = FOREGROUND_RED | FOREGROUND_BLUE;
  QUILL_API static inline WORD const cyan = FOREGROUND_GREEN | FOREGROUND_BLUE;
  QUILL_API static inline WORD const white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

  QUILL_API static inline WORD const on_red = BACKGROUND_RED;
  QUILL_API static inline WORD const on_green = BACKGROUND_GREEN;
  QUILL_API static inline WORD const on_yellow = BACKGROUND_RED | BACKGROUND_GREEN;
  QUILL_API static inline WORD const on_blue = BACKGROUND_BLUE;
  QUILL_API static inline WORD const on_magenta = BACKGROUND_RED | BACKGROUND_BLUE;
  QUILL_API static inline WORD const on_cyan = BACKGROUND_GREEN | BACKGROUND_BLUE;
  QUILL_API static inline WORD const on_white = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_BLUE;

private:
  friend class ConsoleHandler;

  void _set_can_use_colours(FILE* file) noexcept
  {
    _can_use_colours = detail::is_in_terminal(file) && detail::is_colour_terminal();
  }

private:
  std::array<WORD, 10> _colours{}; /**< Colours per log level */
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
    using log_lvl_t = std::underlying_type<LogLevel>::type;
    auto const log_lvl = static_cast<log_lvl_t>(log_level);
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
    using log_lvl_t = std::underlying_type<LogLevel>::type;
    auto const log_lvl = static_cast<log_lvl_t>(log_level);
    return _colours[log_lvl];
  }

  // Formatting codes
  QUILL_API static inline std::string const reset{"\033[0m"};
  QUILL_API static inline std::string const bold{"\033[1m"};
  QUILL_API static inline std::string const dark{"\033[2m"};
  QUILL_API static inline std::string const underline{"\033[4m"};
  QUILL_API static inline std::string const blink{"\033[5m"};
  QUILL_API static inline std::string const reverse{"\033[7m"};
  QUILL_API static inline std::string const concealed{"\033[8m"};
  QUILL_API static inline std::string const clear_line{"\033[K"};

  // Foreground colors
  QUILL_API static inline std::string const black{"\033[30m"};
  QUILL_API static inline std::string const red{"\033[31m"};
  QUILL_API static inline std::string const green{"\033[32m"};
  QUILL_API static inline std::string const yellow{"\033[33m"};
  QUILL_API static inline std::string const blue{"\033[34m"};
  QUILL_API static inline std::string const magenta{"\033[35m"};
  QUILL_API static inline std::string const cyan{"\033[36m"};
  QUILL_API static inline std::string const white{"\033[37m"};

  /// Background colors
  QUILL_API static inline std::string const on_black{"\033[40m"};
  QUILL_API static inline std::string const on_red{"\033[41m"};
  QUILL_API static inline std::string const on_green{"\033[42m"};
  QUILL_API static inline std::string const on_yellow{"\033[43m"};
  QUILL_API static inline std::string const on_blue{"\033[44m"};
  QUILL_API static inline std::string const on_magenta{"\033[45m"};
  QUILL_API static inline std::string const on_cyan{"\033[46m"};
  QUILL_API static inline std::string const on_white{"\033[47m"};

  /// Bold colors
  QUILL_API static inline std::string const yellow_bold{"\033[33m\033[1m"};
  QUILL_API static inline std::string const red_bold{"\033[31m\033[1m"};
  QUILL_API static inline std::string const bold_on_red{"\033[1m\033[41m"};

private:
  friend class ConsoleHandler;

  void _set_can_use_colours(FILE* file) noexcept
  {
    _can_use_colours = detail::is_in_terminal(file) && detail::is_colour_terminal();
  }

private:
  std::array<std::string, 10> _colours{}; /**< Colours per log level */
  bool _using_colours{false};
  bool _can_use_colours{false};
};
#endif

/**
 * Creates a new instance of the FileHandler class.
 * The specified file is opened and used as the stream for logging.
 * If mode is not specified, "a" is used.
 * By default, the file grows indefinitely.
 */
class ConsoleHandler : public StreamHandler
{
public:
  ConsoleHandler(std::string const& stream, FILE* file, ConsoleColours const& console_colours)
    : StreamHandler{stream, file}, _console_colours(console_colours)
  {
    // In this ctor we take a full copy of console_colours and in our instance we modify it
    _console_colours._set_can_use_colours(_file);
  }

  ~ConsoleHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event transit_Event
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message, TransitEvent const& log_event) override
  {
#if defined(_WIN32)
    if (_console_colours.using_colours())
    {
      WORD const colour_code = _console_colours.colour_code(log_event.macro_metadata->log_level());

      // Set foreground colour and store the original attributes
      WORD const orig_attribs = _set_foreground_colour(colour_code);

      auto out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(_file)));

      // Write to console
      bool const write_to_console =
        WriteConsoleA(out_handle, formatted_log_message.data(),
                      static_cast<DWORD>(formatted_log_message.size()), nullptr, nullptr);

      if (QUILL_UNLIKELY(!write_to_console))
      {
        auto const error = std::error_code(GetLastError(), std::system_category());
        std::ostringstream error_msg;
        error_msg << "WriteConsoleA failed with error message "
                  << "\"" << error.message() << "\", errno \"" << error.value() << "\"";
        QUILL_THROW(QuillError{error_msg.str()});
      }

      // reset to orig colors
      bool const set_text_attr = SetConsoleTextAttribute(out_handle, orig_attribs);
      if (QUILL_UNLIKELY(!set_text_attr))
      {
        auto const error = std::error_code(GetLastError(), std::system_category());
        std::ostringstream error_msg;
        error_msg << "SetConsoleTextAttribute failed with error message "
                  << "\"" << error.message() << "\", errno \"" << error.value() << "\"";
        QUILL_THROW(QuillError{error_msg.str()});
      }
    }
    else
    {
      // Write record to file
      StreamHandler::write(formatted_log_message, log_event);
    }
#else
    if (_console_colours.can_use_colours())
    {
      // Write colour code
      std::string const& colour_code = _console_colours.colour_code(log_event.macro_metadata->log_level());

      detail::fwrite_fully(colour_code.data(), sizeof(char), colour_code.size(), _file);
    }

    // Write record to file
    StreamHandler::write(formatted_log_message, log_event);

    if (_console_colours.can_use_colours())
    {
      detail::fwrite_fully(ConsoleColours::reset.data(), sizeof(char), ConsoleColours::reset.size(), _file);
    }
#endif
  }

  /**
   * Used internally to enable the console colours on "stdout" handler which is already
   * created by default without during construction.
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
      std::ostringstream error_msg;
      error_msg << "GetConsoleScreenBufferInfo failed with error message "
                << "\"" << error.message() << "\", errno \"" << error.value() << "\"";
      QUILL_THROW(QuillError{error_msg.str()});
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
      std::ostringstream error_msg;
      error_msg << "SetConsoleTextAttribute failed with error message "
                << "\"" << error.message() << "\", errno \"" << error.value() << "\"";
      QUILL_THROW(QuillError{error_msg.str()});
    }

    return orig_buffer_info.wAttributes; // return orig attribs
  }
#endif

protected:
  // protected in case someone wants to derive from this class and create a custom one, e.g. for json logging to stdout
  ConsoleColours _console_colours;
};
} // namespace quill
