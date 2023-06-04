/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/handlers/StreamHandler.h" // for StreamHandler
#include <array>
#include <string> // for string

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
  ConsoleColours();

  ~ConsoleColours() = default;

  /**
   * Sets some default colours for terminal
   */
  void set_default_colours() noexcept;

  /**
   * Sets a custom colour per log level
   * @param log_level the log level
   * @param colour the colour
   */
  void set_colour(LogLevel log_level, WORD colour) noexcept;

  /**
   * @return true if we are in terminal and have also enabled colours
   */
  QUILL_NODISCARD bool can_use_colours() const noexcept;

  /**
   * @return true if we have setup colours
   */
  QUILL_NODISCARD bool using_colours() const noexcept;

  /**
   * The colour for the given log level
   * @param log_level the message log level
   * @return the configured colour for this log level
   */
  QUILL_NODISCARD WORD colour_code(LogLevel log_level) const noexcept;

  QUILL_API static const WORD bold;

  // Foreground colors
  QUILL_API static const WORD black;
  QUILL_API static const WORD red;
  QUILL_API static const WORD green;
  QUILL_API static const WORD yellow;
  QUILL_API static const WORD blue;
  QUILL_API static const WORD magenta;
  QUILL_API static const WORD cyan;
  QUILL_API static const WORD white;

  // Background colors
  QUILL_API static const WORD on_black;
  QUILL_API static const WORD on_red;
  QUILL_API static const WORD on_green;
  QUILL_API static const WORD on_yellow;
  QUILL_API static const WORD on_blue;
  QUILL_API static const WORD on_magenta;
  QUILL_API static const WORD on_cyan;
  QUILL_API static const WORD on_white;

private:
  friend class ConsoleHandler;

  void _set_can_use_colours(FILE* file) noexcept;

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
  ConsoleColours();

  ~ConsoleColours() = default;

  /**
   * Sets some default colours for terminal
   */
  void set_default_colours() noexcept;

  /**
   * Sets a custom colour per log level
   * @param log_level the log level
   * @param colour the colour
   */
  void set_colour(LogLevel log_level, std::string const& colour) noexcept;

  /**
   * @return true if we are in terminal and have also enabled colours
   */
  QUILL_NODISCARD bool can_use_colours() const noexcept;

  /**
   * @return true if we have setup colours
   */
  QUILL_NODISCARD bool using_colours() const noexcept;

  /**
   * The colour for the given log level
   * @param log_level the message log level
   * @return the configured colour for this log level
   */
  QUILL_NODISCARD std::string const& colour_code(LogLevel log_level) const noexcept;

  // Formatting codes
  QUILL_API static const std::string reset;
  QUILL_API static const std::string bold;
  QUILL_API static const std::string dark;
  QUILL_API static const std::string underline;
  QUILL_API static const std::string blink;
  QUILL_API static const std::string reverse;
  QUILL_API static const std::string concealed;
  QUILL_API static const std::string clear_line;

  // Foreground colors
  QUILL_API static const std::string black;
  QUILL_API static const std::string red;
  QUILL_API static const std::string green;
  QUILL_API static const std::string yellow;
  QUILL_API static const std::string blue;
  QUILL_API static const std::string magenta;
  QUILL_API static const std::string cyan;
  QUILL_API static const std::string white;

  // Background colors
  QUILL_API static const std::string on_black;
  QUILL_API static const std::string on_red;
  QUILL_API static const std::string on_green;
  QUILL_API static const std::string on_yellow;
  QUILL_API static const std::string on_blue;
  QUILL_API static const std::string on_magenta;
  QUILL_API static const std::string on_cyan;
  QUILL_API static const std::string on_white;

  // Bold colors
  QUILL_API static const std::string yellow_bold;
  QUILL_API static const std::string red_bold;
  QUILL_API static const std::string bold_on_red;

private:
  friend class ConsoleHandler;

  void _set_can_use_colours(FILE* file) noexcept;

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
  ConsoleHandler(std::string stream, FILE* file, ConsoleColours const& console_colours);

  ~ConsoleHandler() override = default;

  /**
   * Write a formatted log message to the stream
   * @param formatted_log_message input log message to write
   * @param log_event transit_Event
   */
  QUILL_ATTRIBUTE_HOT void write(fmt_buffer_t const& formatted_log_message, TransitEvent const& log_event) override;

  /**
   * Used internally to enable the console colours on "stdout" handler which is already
   * created by default without during construction.
   */
  QUILL_ATTRIBUTE_COLD void enable_console_colours() noexcept;

private:
#if defined(_WIN32)
  QUILL_NODISCARD ConsoleColours::WORD _set_foreground_colour(ConsoleColours::WORD attributes);
#endif

private:
  ConsoleColours _console_colours;
};
} // namespace quill