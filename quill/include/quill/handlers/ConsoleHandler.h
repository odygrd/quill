/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/LogLevel.h"
#include "quill/detail/misc/Attributes.h"
#include "quill/detail/misc/Common.h"     // for filename_t
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

  static const WORD bold;

  // Foreground colors
  static const WORD black;
  static const WORD red;
  static const WORD green;
  static const WORD yellow;
  static const WORD blue;
  static const WORD magenta;
  static const WORD cyan;
  static const WORD white;

  // Background colors
  static const WORD on_black;
  static const WORD on_red;
  static const WORD on_green;
  static const WORD on_yellow;
  static const WORD on_blue;
  static const WORD on_magenta;
  static const WORD on_cyan;
  static const WORD on_white;

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
  static const std::string reset;
  static const std::string bold;
  static const std::string dark;
  static const std::string underline;
  static const std::string blink;
  static const std::string reverse;
  static const std::string concealed;
  static const std::string clear_line;

  // Foreground colors
  static const std::string black;
  static const std::string red;
  static const std::string green;
  static const std::string yellow;
  static const std::string blue;
  static const std::string magenta;
  static const std::string cyan;
  static const std::string white;

  // Background colors
  static const std::string on_black;
  static const std::string on_red;
  static const std::string on_green;
  static const std::string on_yellow;
  static const std::string on_blue;
  static const std::string on_magenta;
  static const std::string on_cyan;
  static const std::string on_white;

  // Bold colors
  static const std::string yellow_bold;
  static const std::string red_bold;
  static const std::string bold_on_red;

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
  ConsoleHandler(filename_t stream, FILE* file, ConsoleColours const& console_colours);

  ~ConsoleHandler() override = default;

  /**
   * Write a formatted log record to the stream
   * @param formatted_log_record input log record to write
   * @param log_record_timestamp log record timestamp
   */
  QUILL_ATTRIBUTE_HOT void write(fmt::memory_buffer const& formatted_log_record,
                                 std::chrono::nanoseconds log_record_timestamp,
                                 LogLevel log_message_severity) override;

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