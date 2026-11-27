/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/SourceLocation.h"

#ifndef QUILL_MODULE
  #include <cstdint>
  #include <string>
  #include <string_view>
  #include <type_traits>
  #include <utility>
#endif

QUILL_BEGIN_NAMESPACE

QUILL_BEGIN_EXPORT

/**
 * # Macro-Free Logging Interface
 *
 * This file provides an alternative way to log messages without using macros, offering
 * cleaner code in some scenarios.
 *
 * ## Requirements
 *
 * The macro-free mode is implemented using __builtin_FILE(), __builtin_FUNCTION(), __builtin_LINE()
 * which may vary by compiler.
 *
 * ## Performance Trade-offs
 *
 * With C++20, a call whose format string is a string literal and that has no tags measures and
 * scans the format string and its source location at compile time, and its record refers to them as
 * the record of a macro does. Any other format string, and every one in C++17, is copied with the
 * tags on each call, and the backend measures and scans it for each record.
 *
 * Compared to macros, function-based calls still:
 * 1. Always evaluate their arguments even when the log level is disabled
 * 2. Cannot be completely compiled out with QUILL_COMPILE_ACTIVE_LOG_LEVEL_<LEVEL>
 */

namespace detail
{
/**
 * The format string of a macro-free call and the source location of the call
 */
#if defined(__cpp_consteval) && (__cpp_consteval >= 201811L)
inline namespace literal_formats
#else
inline namespace copied_formats
#endif
{
struct FormatString
{
#if defined(__cpp_consteval) && (__cpp_consteval >= 201811L)
  static constexpr bool supports_literal_records = true;

  /**
   * A string literal or a constexpr array with static storage, which the record refers to. Another
   * const char array, e.g. a local one, does not compile: pass a pointer to it, e.g. +fmt, to copy it
   */
  consteval FormatString(char const volatile* fmt, SourceLocation location = SourceLocation::current()) noexcept
    : metadata{const_cast<char const*>(fmt), location.file_name(), location.function_name(), 0},
      line{static_cast<uint32_t>(location.line())}
  {
    // A longer one is copied, also so that the scan does not exceed the constexpr limits of the compiler
    auto const text = detail::c_string_view(metadata.fmt);
    if (text.size() <= StaticRuntimeMetadata::max_fmt_len)
    {
      MacroMetadata const facts{detail::c_string_view(location.file_name()),
                                location.function_name(),
                                text,
                                nullptr,
                                LogLevel::None,
                                MacroMetadata::Event::Log};

      std::string_view const file_path = facts.full_path();

      // full_path() ends at a ':' in the file name, then the call copies its source location
      if (location.file_name()[file_path.size()] == '\0')
      {
        metadata.packed =
          StaticRuntimeMetadata::pack(line, facts.message_format_size(), file_path.size(),
                                      file_path.size() - facts.file_name().size(), facts.format_flags());
      }
    }
  }
#else
  static constexpr bool supports_literal_records = false;

  /** Any format string, copied by each call */
  FormatString(char const* fmt, SourceLocation location = SourceLocation::current()) noexcept
    : metadata{fmt, location.file_name(), location.function_name(), 0},
      line{static_cast<uint32_t>(location.line())}
  {
  }
#endif

  /**
   * A format string that is copied by each call, e.g. a char const*, a char array or an object
   * that converts to char const*. A const char array takes the constructor above: C++20 excludes
   * it here, and C++17 prefers the non-template constructor when both match equally well.
   *
   * The C++20 constraint excludes a const char array of known size; an extern array of unknown size
   * has no sizeof and is copied, as it cannot be read at compile time. That check uses no concept,
   * type trait or std::addressof, which compilers instantiate once per array size, and no
   * operator&, which a format wrapper may overload. is_convertible_v is only checked when the
   * first part holds
   */
#if defined(__cpp_consteval) && (__cpp_consteval >= 201811L)
  template <typename T>
    requires((!requires(T& value) {
               sizeof(value);
               static_cast<char const(*)[]>(__builtin_addressof(value));
             }) || requires(T& value) { static_cast<char(*)[]>(__builtin_addressof(value)); }) &&
              std::is_convertible_v<T, char const*>
#else
  template <typename T, typename = decltype(std::declval<char const*&>() = std::declval<T>())>
#endif
  FormatString(T&& fmt, SourceLocation location = SourceLocation::current())
    : metadata{static_cast<T&&>(fmt), location.file_name(), location.function_name(), 0},
      line{static_cast<uint32_t>(location.line())}
  {
  }

  /** packed is 0 when the call copies its format string */
  StaticRuntimeMetadata metadata;
  uint32_t line;
};
}

template <typename TLogger, typename... Args>
QUILL_ATTRIBUTE_HOT void log(TLogger* logger, LogLevel log_level, FormatString const& fmt,
                             char const* tags, Args&&... args)
{
  if (QUILL_UNLIKELY(!logger))
  {
    return;
  }

  if (logger->should_log_statement(log_level))
  {
    logger->log_statement_macro_free(log_level, fmt, tags, std::forward<Args>(args)...);
  }
}
} // namespace detail

/**
 * Tag structure for log messages that supports multiple tags
 */
struct Tags
{
  Tags() = default;

  /**
   * Constructor for one or more tags
   * @param first_tag Tag to add to the log message
   * @param rest_tags Tag to add to the log message
   */
  template <typename FirstTag, typename... RestTags>
  explicit Tags(FirstTag const& first_tag, RestTags const&... rest_tags)
  {
    append_tag(first_tag);
    (append_tag(rest_tags), ...);
    _append_trailing_space_if_needed();
  }

  // Get the combined tags string
  QUILL_NODISCARD char const* value() const noexcept { return _value.c_str(); }

private:
  std::string _value;

  void append_tag(char const* tag)
  {
    if (QUILL_UNLIKELY(!tag))
    {
      return;
    }

    append_tag(std::string_view{tag});
  }

  void append_tag(std::string const& tag) { append_tag(std::string_view{tag}); }

  void append_tag(std::string_view tag)
  {
    _value += _value.empty() ? "#" : " #";
    if (!tag.empty())
    {
      _value.append(tag.data(), tag.size());
    }
  }

  void _append_trailing_space_if_needed()
  {
    if (!_value.empty())
    {
      _value += ' ';
    }
  }
};

template <typename TLogger, typename... Args>
struct tracel3
{
  tracel3(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::TraceL3, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  tracel3(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::TraceL3, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  tracel3(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
          SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::TraceL3, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
tracel3(TLogger*, detail::FormatString const&, Args&&...) -> tracel3<TLogger, Args...>;

template <typename TLogger, typename... Args>
tracel3(TLogger*, Tags const& tags, char const*, Args&&...) -> tracel3<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct tracel2
{
  tracel2(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::TraceL2, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  tracel2(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::TraceL2, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  tracel2(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
          SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::TraceL2, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
tracel2(TLogger*, detail::FormatString const&, Args&&...) -> tracel2<TLogger, Args...>;

template <typename TLogger, typename... Args>
tracel2(TLogger*, Tags const& tags, char const*, Args&&...) -> tracel2<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct tracel1
{
  tracel1(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::TraceL1, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  tracel1(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::TraceL1, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  tracel1(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
          SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::TraceL1, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
tracel1(TLogger*, detail::FormatString const&, Args&&...) -> tracel1<TLogger, Args...>;

template <typename TLogger, typename... Args>
tracel1(TLogger*, Tags const& tags, char const*, Args&&...) -> tracel1<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct debug
{
  debug(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Debug, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  debug(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Debug, detail::FormatString{fmt.metadata.fmt, location}, nullptr,
                std::forward<Args>(args)...);
  }

  debug(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
        SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Debug, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
debug(TLogger*, detail::FormatString const&, Args&&...) -> debug<TLogger, Args...>;

template <typename TLogger, typename... Args>
debug(TLogger*, Tags const& tags, char const*, Args&&...) -> debug<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct info
{
  info(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Info, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  info(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Info, detail::FormatString{fmt.metadata.fmt, location}, nullptr,
                std::forward<Args>(args)...);
  }

  info(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
       SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Info, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
info(TLogger*, detail::FormatString const&, Args&&...) -> info<TLogger, Args...>;

template <typename TLogger, typename... Args>
info(TLogger*, Tags const& tags, char const*, Args&&...) -> info<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct notice
{
  notice(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Notice, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  notice(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Notice, detail::FormatString{fmt.metadata.fmt, location}, nullptr,
                std::forward<Args>(args)...);
  }

  notice(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
         SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Notice, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
notice(TLogger*, detail::FormatString const&, Args&&...) -> notice<TLogger, Args...>;

template <typename TLogger, typename... Args>
notice(TLogger*, Tags const& tags, char const*, Args&&...) -> notice<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct warning
{
  warning(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Warning, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  warning(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Warning, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  warning(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
          SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Warning, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
warning(TLogger*, detail::FormatString const&, Args&&...) -> warning<TLogger, Args...>;

template <typename TLogger, typename... Args>
warning(TLogger*, Tags const& tags, char const*, Args&&...) -> warning<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct error
{
  error(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Error, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  error(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Error, detail::FormatString{fmt.metadata.fmt, location}, nullptr,
                std::forward<Args>(args)...);
  }

  error(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
        SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Error, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
error(TLogger*, detail::FormatString const&, Args&&...) -> error<TLogger, Args...>;

template <typename TLogger, typename... Args>
error(TLogger*, Tags const& tags, char const*, Args&&...) -> error<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct critical
{
  critical(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Critical, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  critical(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Critical, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  critical(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
           SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Critical, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
critical(TLogger*, detail::FormatString const&, Args&&...) -> critical<TLogger, Args...>;

template <typename TLogger, typename... Args>
critical(TLogger*, Tags const& tags, char const*, Args&&...) -> critical<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct backtrace
{
  backtrace(TLogger* logger, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, LogLevel::Backtrace, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  backtrace(TLogger* logger, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, LogLevel::Backtrace, detail::FormatString{fmt.metadata.fmt, location},
                nullptr, std::forward<Args>(args)...);
  }

  backtrace(TLogger* logger, Tags const& tags, char const* fmt, Args&&... args,
            SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, LogLevel::Backtrace, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
backtrace(TLogger*, detail::FormatString const&, Args&&...) -> backtrace<TLogger, Args...>;

template <typename TLogger, typename... Args>
backtrace(TLogger*, Tags const& tags, char const*, Args&&...) -> backtrace<TLogger, Args...>;

template <typename TLogger, typename... Args>
struct log
{
  log(TLogger* logger, LogLevel log_level, detail::FormatString const& fmt, Args&&... args)
  {
    detail::log(logger, log_level, fmt, nullptr, std::forward<Args>(args)...);
  }

  /** Logs with `location`. A literal format string is then copied */
  log(TLogger* logger, LogLevel log_level, detail::FormatString const& fmt, Args&&... args, SourceLocation location)
  {
    detail::log(logger, log_level, detail::FormatString{fmt.metadata.fmt, location}, nullptr,
                std::forward<Args>(args)...);
  }

  log(TLogger* logger, LogLevel log_level, Tags const& tags, char const* fmt, Args&&... args,
      SourceLocation location = SourceLocation::current())
  {
    detail::log(logger, log_level, detail::FormatString{fmt, location}, tags.value(),
                std::forward<Args>(args)...);
  }
};

template <typename TLogger, typename... Args>
log(TLogger*, LogLevel, detail::FormatString const&, Args&&...) -> log<TLogger, Args...>;

template <typename TLogger, typename... Args>
log(TLogger*, LogLevel, Tags const& tags, char const*, Args&&...) -> log<TLogger, Args...>;

QUILL_END_EXPORT

QUILL_END_NAMESPACE
