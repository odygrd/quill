/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/TweakMe.h"
#include "quill/detail/misc/Attributes.h"

#include "quill/Fmt.h"
#include <functional>
#include <sstream>
#include <string>

// Config Options
#define QUILL_LOG_LEVEL_TRACE_L3 0
#define QUILL_LOG_LEVEL_TRACE_L2 1
#define QUILL_LOG_LEVEL_TRACE_L1 2
#define QUILL_LOG_LEVEL_DEBUG 3
#define QUILL_LOG_LEVEL_INFO 4
#define QUILL_LOG_LEVEL_WARNING 5
#define QUILL_LOG_LEVEL_ERROR 6
#define QUILL_LOG_LEVEL_CRITICAL 7
#define QUILL_LOG_LEVEL_NONE 8

namespace quill::detail
{
enum class QueueType
{
  UnboundedBlocking,
  UnboundedDropping,
  UnboundedNoMaxLimit,
  BoundedBlocking,
  BoundedNonBlocking
};
}

#if defined(QUILL_USE_BOUNDED_QUEUE)
  #define QUILL_QUEUE_TYPE quill::detail::QueueType::BoundedNonBlocking
#elif defined(QUILL_USE_BOUNDED_BLOCKING_QUEUE)
  #define QUILL_QUEUE_TYPE quill::detail::QueueType::BoundedBlocking
#elif defined(QUILL_USE_UNBOUNDED_NO_MAX_LIMIT_QUEUE)
  #define QUILL_QUEUE_TYPE quill::detail::QueueType::UnboundedNoMaxLimit
#elif defined(QUILL_USE_UNBOUNDED_DROPPING_QUEUE)
  #define QUILL_QUEUE_TYPE quill::detail::QueueType::UnboundedDropping
#else
  #define QUILL_QUEUE_TYPE quill::detail::QueueType::UnboundedBlocking
#endif

/**
 * Common type definitions etc
 */
#if !defined(QUILL_ACTIVE_LOG_LEVEL)
  #define QUILL_ACTIVE_LOG_LEVEL QUILL_LOG_LEVEL_TRACE_L3
#endif

#if !defined(QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS)
  #define QUILL_BLOCKING_QUEUE_RETRY_INTERVAL_NS 800
#endif

/**
 * Convert number to string
 */
#define QUILL_AS_STR(x) #x
#define QUILL_STRINGIFY(x) QUILL_AS_STR(x)

/**
 * Likely
 */
#if defined(__GNUC__)
  #define QUILL_LIKELY(x) (__builtin_expect((x), 1))
  #define QUILL_UNLIKELY(x) (__builtin_expect((x), 0))
#else
  #define QUILL_LIKELY(x) (x)
  #define QUILL_UNLIKELY(x) (x)
#endif

/**
 * Require check
 */
#define QUILL_REQUIRE(expression, error)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (QUILL_UNLIKELY(!(expression)))                                                             \
    {                                                                                              \
      printf("Quill fatal error: %s (%s:%d)\n", error, __FILE__, __LINE__);                        \
      std::abort();                                                                                \
    }                                                                                              \
  } while (0)

namespace quill
{
namespace detail
{
/**
 * Cache line size
 */
static constexpr size_t CACHE_LINE_SIZE{64u};
static constexpr size_t CACHE_LINE_ALIGNED{2 * CACHE_LINE_SIZE};

constexpr bool detect_structured_log_template(std::string_view fmt)
{
  uint32_t pos{0};
  bool found_named_arg{false};

  // Iterates the format string and checks if any characters are contained inside `{}`
  while (pos < fmt.length())
  {
    if (fmt[pos] == '{')
    {
      ++pos; // consume {
      if (pos >= fmt.length())
      {
        break;
      }

      // first character after the {
      auto fc = fmt[pos];
      if (fc == '{')
      {
        // this means first '{' was escaped, so we ignore both of them
        ++pos;
        continue;
      }

      // else look for the next '}'
      uint32_t char_cnt{0};
      while (pos < fmt.length())
      {
        if (fmt[pos] == '}')
        {
          ++pos; // consume }
          if (pos >= fmt.length())
          {
            break;
          }

          if (fmt[pos] == '}')
          {
            // this means first '}', was escaped ignore it
            ++pos;
            ++char_cnt;
            continue;
          }
          else
          {
            // we found '{' match, we can break
            break;
          }
        }

        ++pos;
        ++char_cnt;
      }

      if ((char_cnt != 0) && ((fc >= 'a' && fc <= 'z') || (fc >= 'A' && fc <= 'Z')))
      {
        found_named_arg = true;
      }
    }
    ++pos;
  }

  return found_named_arg;
}

constexpr bool detect_structured_log_template(std::wstring_view)
{
  // structured log for wide chars is not supported. We always return false here, if the user provides a structured log with wide characters
  // we expected the fmt compile time format check to fail
  return false;
}

constexpr void QUILL_PRINTF_FORMAT_ATTRIBUTE(1, 2) check_printf_args(char const*, ...) {}

template <typename... Args, typename S>
constexpr bool check_printf_format_string(S format_str)
{
  using char_t = typename S::char_type;
  constexpr auto format = fmtquill::basic_string_view<char_t>(format_str);
  size_t num_specifiers = 0;

  for (auto it = format.begin(); it != format.end(); ++it)
  {
    if (*it == '%')
    {
      ++num_specifiers;
    }
  }

  if (num_specifiers > sizeof...(Args))
  {
    throw std::runtime_error{
      "Invalid printf format: format string does not match number of arguments"};
  }

  return true;
}

} // namespace detail

using transit_event_fmt_buffer_t = fmtquill::basic_memory_buffer<char, 1>;
using fmt_buffer_t = fmtquill::basic_memory_buffer<char, 2048>;

/**
 * Enum to select a timezone
 */
enum class Timezone : uint8_t
{
  LocalTime,
  GmtTime
};

/**
 * Enum for the used clock type
 */
enum TimestampClockType : uint8_t
{
  Tsc = 0,
  System,
  Custom
};

/**
 * backend worker thread error handler type
 */
using backend_worker_notification_handler_t = std::function<void(std::string const&)>;

} // namespace quill

#if !defined(QUILL_HAS_FILESYSTEM) && !defined(QUILL_HAS_EXPERIMENTAL_FILESYSTEM)
  #if defined(__cpp_lib_filesystem)
    #define QUILL_HAS_FILESYSTEM 1
  #elif defined(__cpp_lib_experimental_filesystem)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #elif !defined(__has_include)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #elif __has_include(<filesystem>)
    #define QUILL_HAS_FILESYSTEM 1
  #elif __has_include(<experimental/filesystem>)
    #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 1
  #endif

  // std::filesystem does not work on MinGW GCC 8: https://sourceforge.net/p/mingw-w64/bugs/737/
  #if defined(__MINGW32__) && defined(__GNUC__) && __GNUC__ == 8
    #undef QUILL_HAS_FILESYSTEM
    #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif

  // no filesystem support before GCC 8: https://en.cppreference.com/w/cpp/compiler_support
  #if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
    #undef QUILL_HAS_FILESYSTEM
  // #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif

  // no filesystem support before Clang 7: https://en.cppreference.com/w/cpp/compiler_support
  #if defined(__clang_major__) && __clang_major__ < 7
    #undef QUILL_HAS_FILESYSTEM
    #undef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #endif
#endif

#ifndef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #define QUILL_HAS_EXPERIMENTAL_FILESYSTEM 0
#endif

#ifndef QUILL_HAS_FILESYSTEM
  #define QUILL_HAS_FILESYSTEM 0
#endif

#if QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  #include <experimental/filesystem>
namespace quill
{
namespace fs = std::experimental::filesystem;
} // namespace quill
#elif QUILL_HAS_FILESYSTEM
  #include <filesystem>
namespace quill
{
namespace fs = std::filesystem;
} // namespace quill
#endif
