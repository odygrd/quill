#include "quill/handlers/ConsoleHandler.h"
#include "quill/detail/misc/FileUtilities.h" // for fwrite_fully
#include "quill/detail/misc/Os.h"
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
WORD const ConsoleColours::bold = FOREGROUND_INTENSITY;

WORD const ConsoleColours::black = 0;
WORD const ConsoleColours::red = FOREGROUND_RED;
WORD const ConsoleColours::green = FOREGROUND_GREEN;
WORD const ConsoleColours::yellow = FOREGROUND_RED | FOREGROUND_GREEN;
WORD const ConsoleColours::blue = FOREGROUND_BLUE;
WORD const ConsoleColours::magenta = FOREGROUND_RED | FOREGROUND_BLUE;
WORD const ConsoleColours::cyan = FOREGROUND_GREEN | FOREGROUND_BLUE;
WORD const ConsoleColours::white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

WORD const ConsoleColours::on_red = BACKGROUND_RED;
WORD const ConsoleColours::on_green = BACKGROUND_GREEN;
WORD const ConsoleColours::on_yellow = BACKGROUND_RED | BACKGROUND_GREEN;
WORD const ConsoleColours::on_blue = BACKGROUND_BLUE;
WORD const ConsoleColours::on_magenta = BACKGROUND_RED | BACKGROUND_BLUE;
WORD const ConsoleColours::on_cyan = BACKGROUND_GREEN | BACKGROUND_BLUE;
WORD const ConsoleColours::on_white = BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_BLUE;

/***/
ConsoleColours::ConsoleColours() { _colours.fill(white); }

/***/
void ConsoleColours::set_default_colours() noexcept
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

/***/
void ConsoleColours::set_colour(LogLevel log_level, WORD colour) noexcept
{
  using log_lvl_t = std::underlying_type<LogLevel>::type;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);
  _colours[log_lvl] = colour;
  _using_colours = true;
}

/***/
bool ConsoleColours::can_use_colours() const noexcept { return _can_use_colours && _using_colours; }

/***/
bool ConsoleColours::using_colours() const noexcept { return _using_colours; }

/***/
WORD ConsoleColours::colour_code(LogLevel log_level) const noexcept
{
  using log_lvl_t = std::underlying_type<LogLevel>::type;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);
  return _colours[log_lvl];
}

/***/
void ConsoleColours::_set_can_use_colours(FILE* file) noexcept
{
  _can_use_colours = detail::is_in_terminal(file) && detail::is_colour_terminal();
}
#else
std::string const ConsoleColours::reset{"\033[0m"};
std::string const ConsoleColours::bold{"\033[1m"};
std::string const ConsoleColours::dark{"\033[2m"};
std::string const ConsoleColours::underline{"\033[4m"};
std::string const ConsoleColours::blink{"\033[5m"};
std::string const ConsoleColours::reverse{"\033[7m"};
std::string const ConsoleColours::concealed{"\033[8m"};
std::string const ConsoleColours::clear_line{"\033[K"};

// Foreground colors
std::string const ConsoleColours::black{"\033[30m"};
std::string const ConsoleColours::red{"\033[31m"};
std::string const ConsoleColours::green{"\033[32m"};
std::string const ConsoleColours::yellow{"\033[33m"};
std::string const ConsoleColours::blue{"\033[34m"};
std::string const ConsoleColours::magenta{"\033[35m"};
std::string const ConsoleColours::cyan{"\033[36m"};
std::string const ConsoleColours::white{"\033[37m"};

/// Background colors
std::string const ConsoleColours::on_black{"\033[40m"};
std::string const ConsoleColours::on_red{"\033[41m"};
std::string const ConsoleColours::on_green{"\033[42m"};
std::string const ConsoleColours::on_yellow{"\033[43m"};
std::string const ConsoleColours::on_blue{"\033[44m"};
std::string const ConsoleColours::on_magenta{"\033[45m"};
std::string const ConsoleColours::on_cyan{"\033[46m"};
std::string const ConsoleColours::on_white{"\033[47m"};

/// Bold colors
std::string const ConsoleColours::yellow_bold{"\033[33m\033[1m"};
std::string const ConsoleColours::red_bold{"\033[31m\033[1m"};
std::string const ConsoleColours::bold_on_red{"\033[1m\033[41m"};

/***/
ConsoleColours::ConsoleColours() { _colours.fill(white); }

/***/
void ConsoleColours::set_default_colours() noexcept
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

/***/
void ConsoleColours::set_colour(LogLevel log_level, std::string const& colour) noexcept
{
  using log_lvl_t = std::underlying_type<LogLevel>::type;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);
  _colours[log_lvl] = colour;
  _using_colours = true;
}

/***/
bool ConsoleColours::can_use_colours() const noexcept { return _can_use_colours && _using_colours; }

/***/
bool ConsoleColours::using_colours() const noexcept { return _using_colours; }

/***/
std::string const& ConsoleColours::colour_code(LogLevel log_level) const noexcept
{
  using log_lvl_t = std::underlying_type<LogLevel>::type;
  auto const log_lvl = static_cast<log_lvl_t>(log_level);
  return _colours[log_lvl];
}

/***/
void ConsoleColours::_set_can_use_colours(FILE* file) noexcept
{
  _can_use_colours = detail::is_in_terminal(file) && detail::is_colour_terminal();
}
#endif

/***/
ConsoleHandler::ConsoleHandler(filename_t stream, FILE* file, ConsoleColours const& console_colours)
  : StreamHandler{std::move(stream), file}, _console_colours(console_colours)
{
  // In this ctor we take a full copy of console_colours and in our instance we modify it
  _console_colours._set_can_use_colours(_file);
}

/***/
void ConsoleHandler::write(fmt::memory_buffer const& formatted_log_record,
                           std::chrono::nanoseconds log_record_timestamp, LogLevel log_message_severity)
{
#if defined(_WIN32)
  if (_console_colours.using_colours())
  {
    WORD const colour_code = _console_colours.colour_code(log_message_severity);

    // Set foreground colour and store the original attributes
    WORD const orig_attribs = _set_foreground_colour(colour_code);

    auto out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(_file)));

    // Write to console
    bool const write_to_console =
      ::WriteConsoleA(out_handle, formatted_log_record.data(),
                      static_cast<DWORD>(formatted_log_record.size()), nullptr, nullptr);

    if (QUILL_UNLIKELY(!write_to_console))
    {
      auto const error = std::error_code(GetLastError(), std::system_category());
      std::ostringstream error_msg;
      error_msg << "WriteConsoleA failed with error message "
                << "\"" << error.message() << "\", errno \"" << error.value() << "\"";
      QUILL_THROW(QuillError{error_msg.str()});
    }

    // reset to orig colors
    bool const set_text_attr = ::SetConsoleTextAttribute(out_handle, orig_attribs);
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
    StreamHandler::write(formatted_log_record, log_record_timestamp, log_message_severity);
  }
#else
  if (_console_colours.can_use_colours())
  {
    // Write colour code
    std::string const& colour_code = _console_colours.colour_code(log_message_severity);

    detail::file_utilities::fwrite_fully(colour_code.data(), sizeof(char), colour_code.size(), _file);
  }

  // Write record to file
  StreamHandler::write(formatted_log_record, log_record_timestamp, log_message_severity);

  if (_console_colours.can_use_colours())
  {
    detail::file_utilities::fwrite_fully(ConsoleColours::reset.data(), sizeof(char),
                                         ConsoleColours::reset.size(), _file);
  }
#endif
}

/***/
void ConsoleHandler::enable_console_colours() noexcept { _console_colours.set_default_colours(); }

#if defined(_WIN32)
/***/
ConsoleColours::WORD ConsoleHandler::_set_foreground_colour(ConsoleColours::WORD attributes)
{
  CONSOLE_SCREEN_BUFFER_INFO orig_buffer_info;
  auto out_handle = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(_file)));

  bool const screen_buffer_info = ::GetConsoleScreenBufferInfo(out_handle, &orig_buffer_info);
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
  bool const console_text_attr = ::SetConsoleTextAttribute(out_handle, attributes | back_color);
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

} // namespace quill
