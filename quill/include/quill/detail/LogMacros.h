/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"

#include "quill/Logger.h"
#include <type_traits>

/**
 * A macro to indicate that a user define type is copy_loggable
 */
#define QUILL_COPY_LOGGABLE using copy_loggable = std::true_type

// Main Log Macros
#define QUILL_LOGGER_CALL_NOFN(likelyhood, logger, log_statement_level, fmt, ...)                  \
  do                                                                                               \
  {                                                                                                \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    "n/a",                                                         \
                                    fmt,                                                           \
                                    log_statement_level,                                           \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    false};                                                        \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL(likelyhood, logger, log_statement_level, fmt, ...)                       \
  do                                                                                               \
  {                                                                                                \
    static constexpr char const* function_name = __FUNCTION__;                                     \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    function_name,                                                 \
                                    fmt,                                                           \
                                    log_statement_level,                                           \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    false};                                                        \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_LIMIT(min_interval, likelyhood, logger, log_statement_level, fmt, ...)   \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      thread_local std::chrono::time_point<std::chrono::steady_clock> next_log_time;               \
      auto const now = std::chrono::steady_clock::now();                                           \
                                                                                                   \
      if (now < next_log_time)                                                                     \
      {                                                                                            \
        break;                                                                                     \
      }                                                                                            \
                                                                                                   \
      next_log_time = now + min_interval;                                                          \
      QUILL_LOGGER_CALL(likelyhood, logger, log_statement_level, fmt, ##__VA_ARGS__);              \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, likelyhood, logger, log_statement_level, fmt, ...) \
  do                                                                                                  \
  {                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                               \
    {                                                                                                 \
      thread_local std::chrono::time_point<std::chrono::steady_clock> next_log_time;                  \
      auto const now = std::chrono::steady_clock::now();                                              \
                                                                                                      \
      if (now < next_log_time)                                                                        \
      {                                                                                               \
        break;                                                                                        \
      }                                                                                               \
                                                                                                      \
      limit_us = now + min_interval;                                                                  \
      QUILL_LOGGER_CALL_NOFN(likelyhood, logger, log_statement_level, fmt, ##__VA_ARGS__);            \
    }                                                                                                 \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ...)                                              \
  do                                                                                               \
  {                                                                                                \
    static constexpr char const* function_name = __FUNCTION__;                                     \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    function_name,                                                 \
                                    fmt,                                                           \
                                    quill::LogLevel::Backtrace,                                    \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    false};                                                        \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (QUILL_LIKELY(logger->template should_log<quill::LogLevel::Backtrace>()))                   \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

/**
 * Dynamic runtime log level with a tiny overhead
 * @Note: Prefer using the compile time log level macros
 */
#define QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ...)                                                        \
  do                                                                                                               \
  {                                                                                                                \
    static constexpr char const* function_name = __FUNCTION__;                                                     \
    struct                                                                                                         \
    {                                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                                   \
      {                                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                                     \
                                    __FILE__,                                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                                        \
                                    function_name,                                                                 \
                                    fmt,                                                                           \
                                    quill::LogLevel::Dynamic,                                                      \
                                    quill::MacroMetadata::Event::Log,                                              \
                                    quill::detail::detect_structured_log_template(fmt),                            \
                                    false};                                                                        \
      }                                                                                                            \
    } anonymous_log_message_info;                                                                                  \
                                                                                                                   \
    if (logger->should_log(log_level))                                                                             \
    {                                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(log_level, QUILL_FMT_STRING(fmt), ##__VA_ARGS__); \
    }                                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_NOFN_CFORMAT(likelyhood, logger, log_statement_level, fmt, ...)          \
  do                                                                                               \
  {                                                                                                \
    if (false)                                                                                     \
      quill::detail::check_printf_args(fmt, ##__VA_ARGS__);                                        \
                                                                                                   \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    "n/a",                                                         \
                                    fmt,                                                           \
                                    log_statement_level,                                           \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    true};                                                         \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_CFORMAT(likelyhood, logger, log_statement_level, fmt, ...)               \
  do                                                                                               \
  {                                                                                                \
    if (false)                                                                                     \
      quill::detail::check_printf_args(fmt, ##__VA_ARGS__);                                        \
                                                                                                   \
    static constexpr char const* function_name = __FUNCTION__;                                     \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    function_name,                                                 \
                                    fmt,                                                           \
                                    log_statement_level,                                           \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    true};                                                         \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, likelyhood, logger, log_statement_level, fmt, ...) \
  do                                                                                                     \
  {                                                                                                      \
    if (likelyhood(logger->template should_log<log_statement_level>()))                                  \
    {                                                                                                    \
      thread_local std::chrono::time_point<std::chrono::steady_clock> next_log_time;                     \
      auto const now = std::chrono::steady_clock::now();                                                 \
                                                                                                         \
      if (now < next_log_time)                                                                           \
      {                                                                                                  \
        break;                                                                                           \
      }                                                                                                  \
                                                                                                         \
      next_log_time = now + min_interval;                                                                \
      QUILL_LOGGER_CALL_CFORMAT(likelyhood, logger, log_statement_level, fmt, ##__VA_ARGS__);            \
    }                                                                                                    \
  } while (0)

#define QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, likelyhood, logger, log_statement_level, fmt, ...) \
  do                                                                                                          \
  {                                                                                                           \
    if (likelyhood(logger->template should_log<log_statement_level>()))                                       \
    {                                                                                                         \
      thread_local std::chrono::time_point<std::chrono::steady_clock> next_log_time;                          \
      auto const now = std::chrono::steady_clock::now();                                                      \
                                                                                                              \
      if (now < next_log_time)                                                                                \
      {                                                                                                       \
        break;                                                                                                \
      }                                                                                                       \
                                                                                                              \
      limit_us = now + min_interval;                                                                          \
      QUILL_LOGGER_CALL_NOFN_CFORMAT(likelyhood, logger, log_statement_level, fmt, ##__VA_ARGS__);            \
    }                                                                                                         \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL_CFORMAT(logger, fmt, ...)                                      \
  do                                                                                               \
  {                                                                                                \
    if (false)                                                                                     \
      quill::detail::check_printf_args(fmt, ##__VA_ARGS__);                                        \
                                                                                                   \
    static constexpr char const* function_name = __FUNCTION__;                                     \
    struct                                                                                         \
    {                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                   \
      {                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                     \
                                    __FILE__,                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                        \
                                    function_name,                                                 \
                                    fmt,                                                           \
                                    quill::LogLevel::Backtrace,                                    \
                                    quill::MacroMetadata::Event::Log,                              \
                                    quill::detail::detect_structured_log_template(fmt),            \
                                    true};                                                         \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (QUILL_LIKELY(logger->template should_log<quill::LogLevel::Backtrace>()))                   \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(                                  \
        quill::LogLevel::None, QUILL_FMT_STRING(fmt), ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

/**
 * Dynamic runtime log level with a tiny overhead
 * @Note: Prefer using the compile time log level macros
 */
#define QUILL_DYNAMIC_LOG_CALL_CFORMAT(logger, log_level, fmt, ...)                                                \
  do                                                                                                               \
  {                                                                                                                \
    if (false)                                                                                                     \
      quill::detail::check_printf_args(fmt, ##__VA_ARGS__);                                                        \
                                                                                                                   \
    static constexpr char const* function_name = __FUNCTION__;                                                     \
    struct                                                                                                         \
    {                                                                                                              \
      constexpr quill::MacroMetadata operator()() const noexcept                                                   \
      {                                                                                                            \
        return quill::MacroMetadata{QUILL_STRINGIFY(__LINE__),                                                     \
                                    __FILE__,                                                                      \
                                    __FILE__ ":" QUILL_STRINGIFY(__LINE__),                                        \
                                    function_name,                                                                 \
                                    fmt,                                                                           \
                                    quill::LogLevel::Dynamic,                                                      \
                                    quill::MacroMetadata::Event::Log,                                              \
                                    quill::detail::detect_structured_log_template(fmt),                            \
                                    true};                                                                         \
      }                                                                                                            \
    } anonymous_log_message_info;                                                                                  \
                                                                                                                   \
    if (logger->should_log(log_level))                                                                             \
    {                                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(log_level, QUILL_FMT_STRING(fmt), ##__VA_ARGS__); \
    }                                                                                                              \
  } while (0)

#define QUILL_DYNAMIC_LOG(logger, log_level, fmt, ...)                                             \
  QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ##__VA_ARGS__)

#define QUILL_DYNAMIC_LOG_CFORMAT(logger, log_level, fmt, ...)                                     \
  QUILL_DYNAMIC_LOG_CALL_CFORMAT(logger, log_level, fmt, ##__VA_ARGS__)

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L3
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_NOFN_LIMIT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_CFORMAT(logger, fmt, ...)                                             \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                         \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_NOFN_CFORMAT(logger, fmt, ...)                                        \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                    \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L2
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_NOFN_LIMIT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_CFORMAT(logger, fmt, ...)                                             \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                         \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_NOFN_CFORMAT(logger, fmt, ...)                                        \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                    \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L1
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_NOFN_LIMIT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_CFORMAT(logger, fmt, ...)                                             \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                         \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_NOFN_CFORMAT(logger, fmt, ...)                                        \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                    \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_DEBUG
  #define QUILL_LOG_DEBUG(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_NOFN(logger, fmt, ...)                                                   \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_NOFN_LIMIT(min_interval, logger, fmt, ...)                               \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_CFORMAT(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_CFORMAT(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_NOFN_CFORMAT(logger, fmt, ...)                                           \
    QUILL_LOGGER_CALL_NOFN_CFORMAT(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                       \
    QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, QUILL_UNLIKELY, logger,                     \
                                         quill::LogLevel::Debug, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_DEBUG(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_INFO
  #define QUILL_LOG_INFO(logger, fmt, ...)                                                         \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_NOFN(logger, fmt, ...)                                                    \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_NOFN_LIMIT(min_interval, logger, fmt, ...)                                \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_CFORMAT(logger, fmt, ...)                                                 \
    QUILL_LOGGER_CALL_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_LIMIT_CFORMAT(min_interval, logger, ...)                                  \
    QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_NOFN_CFORMAT(logger, fmt, ...)                                            \
    QUILL_LOGGER_CALL_NOFN_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                        \
    QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger,                       \
                                         quill::LogLevel::Info, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_INFO(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_WARNING
  #define QUILL_LOG_WARNING(logger, fmt, ...)                                                      \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_NOFN(logger, fmt, ...)                                                 \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_NOFN_LIMIT(min_interval, logger, fmt, ...)                             \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_CFORMAT(logger, fmt, ...)                                              \
    QUILL_LOGGER_CALL_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                          \
    QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_NOFN_CFORMAT(logger, fmt, ...)                                         \
    QUILL_LOGGER_CALL_NOFN_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                     \
    QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger,                       \
                                         quill::LogLevel::Warning, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_WARNING(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_ERROR
  #define QUILL_LOG_ERROR(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_NOFN(logger, fmt, ...)                                                   \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_NOFN_LIMIT(min_interval, logger, fmt, ...)                               \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_CFORMAT(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_NOFN_CFORMAT(logger, fmt, ...)                                           \
    QUILL_LOGGER_CALL_NOFN_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                       \
    QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger,                       \
                                         quill::LogLevel::Error, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_ERROR(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
  #define QUILL_LOG_CRITICAL(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_NOFN_LIMIT(min_interval, logger, fmt, ...)                            \
    QUILL_LOGGER_CALL_NOFN_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_CFORMAT(logger, fmt, ...)                                             \
    QUILL_LOGGER_CALL_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                         \
    QUILL_LOGGER_CALL_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_NOFN_CFORMAT(logger, fmt, ...)                                        \
    QUILL_LOGGER_CALL_NOFN_CFORMAT(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                    \
    QUILL_LOGGER_CALL_NOFN_LIMIT_CFORMAT(min_interval, QUILL_LIKELY, logger,                       \
                                         quill::LogLevel::Critical, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_CRITICAL(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_NOFN(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_NOFN_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_NOFN_CFORMAT(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...) (void)0
#endif

#define QUILL_LOG_BACKTRACE(logger, fmt, ...)                                                      \
  QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ##__VA_ARGS__)

#define QUILL_LOG_BACKTRACE_CFORMAT(logger, fmt, ...)                                              \
  QUILL_BACKTRACE_LOGGER_CALL_CFORMAT(logger, fmt, ##__VA_ARGS__)

#if !defined(QUILL_DISABLE_NON_PREFIXED_MACROS) && !defined(QUILL_ROOT_LOGGER_ONLY)
  #define LOG_TRACE_L3(logger, fmt, ...) QUILL_LOG_TRACE_L3(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2(logger, fmt, ...) QUILL_LOG_TRACE_L2(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1(logger, fmt, ...) QUILL_LOG_TRACE_L1(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG(logger, fmt, ...) QUILL_LOG_DEBUG(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO(logger, fmt, ...) QUILL_LOG_INFO(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING(logger, fmt, ...) QUILL_LOG_WARNING(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR(logger, fmt, ...) QUILL_LOG_ERROR(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL(logger, fmt, ...) QUILL_LOG_CRITICAL(logger, fmt, ##__VA_ARGS__)
  #define LOG_BACKTRACE(logger, fmt, ...) QUILL_LOG_BACKTRACE(logger, fmt, ##__VA_ARGS__)
  #define LOG_DYNAMIC(logger, log_level, fmt, ...)                                                 \
    QUILL_DYNAMIC_LOG(logger, log_level, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                          \
    QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                           \
    QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                        \
    QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                          \
    QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                       \
    QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN(logger, fmt, ...) QUILL_LOG_DEBUG_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN(logger, fmt, ...) QUILL_LOG_INFO_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN(logger, fmt, ...) QUILL_LOG_WARNING_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN(logger, fmt, ...) QUILL_LOG_ERROR_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN(logger, fmt, ...) QUILL_LOG_CRITICAL_NOFN(logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_TRACE_L3_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_TRACE_L2_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_TRACE_L1_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOG_DEBUG_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_LIMIT(min_interval, logger, fmt, ...)                                      \
    QUILL_LOG_INFO_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_LIMIT(min_interval, logger, fmt, ...)                                   \
    QUILL_LOG_WARNING_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOG_ERROR_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_CRITICAL_NOFN_LIMIT(min_interval, logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_CFORMAT(logger, fmt, ...)                                                   \
    QUILL_LOG_TRACE_L3_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_CFORMAT(logger, fmt, ...)                                                   \
    QUILL_LOG_TRACE_L2_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_CFORMAT(logger, fmt, ...)                                                   \
    QUILL_LOG_TRACE_L1_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_CFORMAT(logger, fmt, ...) QUILL_LOG_DEBUG_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_CFORMAT(logger, fmt, ...) QUILL_LOG_INFO_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_CFORMAT(logger, fmt, ...)                                                    \
    QUILL_LOG_WARNING_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_CFORMAT(logger, fmt, ...) QUILL_LOG_ERROR_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_CFORMAT(logger, fmt, ...)                                                   \
    QUILL_LOG_CRITICAL_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_BACKTRACE_CFORMAT(logger, fmt, ...)                                                  \
    QUILL_LOG_BACKTRACE_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_DYNAMIC_CFORMAT(logger, log_level, fmt, ...)                                         \
    QUILL_DYNAMIC_LOG_CFORMAT(logger, log_level, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                               \
    QUILL_LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                               \
    QUILL_LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                               \
    QUILL_LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_DEBUG_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                                   \
    QUILL_LOG_INFO_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                                \
    QUILL_LOG_WARNING_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOG_ERROR_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                               \
    QUILL_LOG_CRITICAL_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_CFORMAT(logger, fmt, ...)                                              \
    QUILL_LOG_TRACE_L3_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_CFORMAT(logger, fmt, ...)                                              \
    QUILL_LOG_TRACE_L2_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_CFORMAT(logger, fmt, ...)                                              \
    QUILL_LOG_TRACE_L1_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_CFORMAT(logger, fmt, ...)                                                 \
    QUILL_LOG_DEBUG_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_CFORMAT(logger, fmt, ...)                                                  \
    QUILL_LOG_INFO_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_CFORMAT(logger, fmt, ...)                                               \
    QUILL_LOG_WARNING_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_CFORMAT(logger, fmt, ...)                                                 \
    QUILL_LOG_ERROR_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_CFORMAT(logger, fmt, ...)                                              \
    QUILL_LOG_CRITICAL_NOFN_CFORMAT(logger, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                          \
    QUILL_LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                          \
    QUILL_LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                          \
    QUILL_LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                             \
    QUILL_LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                              \
    QUILL_LOG_INFO_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                           \
    QUILL_LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                             \
    QUILL_LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ...)                          \
    QUILL_LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, logger, fmt, ##__VA_ARGS__)
#elif !defined(QUILL_DISABLE_NON_PREFIXED_MACROS) && defined(QUILL_ROOT_LOGGER_ONLY)
  #define LOG_TRACE_L3(fmt, ...) QUILL_LOG_TRACE_L3(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2(fmt, ...) QUILL_LOG_TRACE_L2(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1(fmt, ...) QUILL_LOG_TRACE_L1(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG(fmt, ...) QUILL_LOG_DEBUG(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO(fmt, ...) QUILL_LOG_INFO(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING(fmt, ...) QUILL_LOG_WARNING(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR(fmt, ...) QUILL_LOG_ERROR(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL(fmt, ...) QUILL_LOG_CRITICAL(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_BACKTRACE(fmt, ...) QUILL_LOG_BACKTRACE(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DYNAMIC(log_level, fmt, ...)                                                         \
    QUILL_DYNAMIC_LOG(quill::get_root_logger(), log_level, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_LIMIT(min_interval, fmt, ...)                                               \
    QUILL_LOG_TRACE_L3_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_LIMIT(min_interval, fmt, ...)                                               \
    QUILL_LOG_TRACE_L2_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_LIMIT(min_interval, fmt, ...)                                               \
    QUILL_LOG_TRACE_L1_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_LIMIT(min_interval, fmt, ...)                                                  \
    QUILL_LOG_DEBUG_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_LIMIT(min_interval, fmt, ...)                                                   \
    QUILL_LOG_INFOv_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_LIMIT(min_interval, fmt, ...)                                                \
    QUILL_LOG_WARNING_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_LIMIT(min_interval, fmt, ...)                                                  \
    QUILL_LOG_ERROR_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_LIMIT(min_interval, fmt, ...)                                               \
    QUILL_LOG_CRITICAL_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN(fmt, ...)                                                              \
    QUILL_LOG_TRACE_L3_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN(fmt, ...)                                                              \
    QUILL_LOG_TRACE_L2_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN(fmt, ...)                                                              \
    QUILL_LOG_TRACE_L1_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN(fmt, ...)                                                                 \
    QUILL_LOG_DEBUG_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN(fmt, ...) QUILL_LOG_INFO_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN(fmt, ...)                                                               \
    QUILL_LOG_WARNING_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN(fmt, ...)                                                                 \
    QUILL_LOG_ERROR_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN(fmt, ...)                                                              \
    QUILL_LOG_CRITICAL_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_LIMIT(min_interval, fmt, ...)                                          \
    QUILL_LOG_TRACE_L3_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_LIMIT(min_interval, fmt, ...)                                          \
    QUILL_LOG_TRACE_L2_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_LIMIT(min_interval, fmt, ...)                                          \
    QUILL_LOG_TRACE_L1_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_LIMIT(min_interval, fmt, ...)                                             \
    QUILL_LOG_DEBUG_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_LIMIT(min_interval, fmt, ...)                                              \
    QUILL_LOG_INFO_NOFN(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_LIMIT(min_interval, fmt, ...)                                           \
    QUILL_LOG_WARNING_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_LIMIT(min_interval, fmt, ...)                                             \
    QUILL_LOG_ERROR_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_LIMIT(min_interval, fmt, ...)                                          \
    QUILL_LOG_CRITICAL_NOFN_LIMIT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_CFORMAT(fmt, ...)                                                           \
    QUILL_LOG_TRACE_L3_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_CFORMAT(fmt, ...)                                                           \
    QUILL_LOG_TRACE_L2_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_CFORMAT(fmt, ...)                                                           \
    QUILL_LOG_TRACE_L1_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_CFORMAT(fmt, ...)                                                              \
    QUILL_LOG_DEBUG_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_CFORMAT(fmt, ...)                                                               \
    QUILL_LOG_INFO_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_CFORMAT(fmt, ...)                                                            \
    QUILL_LOG_WARNING_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_CFORMAT(fmt, ...)                                                              \
    QUILL_LOG_ERROR_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_CFORMAT(fmt, ...)                                                           \
    QUILL_LOG_CRITICAL_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_BACKTRACE_CFORMAT(fmt, ...)                                                          \
    QUILL_LOG_BACKTRACE_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DYNAMIC_CFORMAT(log_level, fmt, ...)                                                 \
    QUILL_DYNAMIC_LOG_CFORMAT(quill::get_root_logger(), log_level, fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, fmt, ...)                                       \
    QUILL_LOG_TRACE_L3_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, fmt, ...)                                       \
    QUILL_LOG_TRACE_L2_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, fmt, ...)                                       \
    QUILL_LOG_TRACE_L1_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_LIMIT_CFORMAT(min_interval, fmt, ...)                                          \
    QUILL_LOG_DEBUG_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_LIMIT_CFORMAT(min_interval, fmt, ...)                                           \
    QUILL_LOG_INFOv_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_LIMIT_CFORMAT(min_interval, fmt, ...)                                        \
    QUILL_LOG_WARNING_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_LIMIT_CFORMAT(min_interval, fmt, ...)                                          \
    QUILL_LOG_ERROR_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_LIMIT_CFORMAT(min_interval, fmt, ...)                                       \
    QUILL_LOG_CRITICAL_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_CFORMAT(fmt, ...)                                                      \
    QUILL_LOG_TRACE_L3_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_CFORMAT(fmt, ...)                                                      \
    QUILL_LOG_TRACE_L2_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_CFORMAT(fmt, ...)                                                      \
    QUILL_LOG_TRACE_L1_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_CFORMAT(fmt, ...)                                                         \
    QUILL_LOG_DEBUG_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_CFORMAT(fmt, ...)                                                          \
    QUILL_LOG_INFO_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_CFORMAT(fmt, ...)                                                       \
    QUILL_LOG_WARNING_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_CFORMAT(fmt, ...)                                                         \
    QUILL_LOG_ERROR_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_CFORMAT(fmt, ...)                                                      \
    QUILL_LOG_CRITICAL_NOFN_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)

  #define LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                  \
    QUILL_LOG_TRACE_L3_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                  \
    QUILL_LOG_TRACE_L2_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                  \
    QUILL_LOG_TRACE_L1_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                     \
    QUILL_LOG_DEBUG_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                      \
    QUILL_LOG_INFO_NOFN_LIMIT_CFORMAT(quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                   \
    QUILL_LOG_WARNING_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                     \
    QUILL_LOG_ERROR_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, fmt, ...)                                  \
    QUILL_LOG_CRITICAL_NOFN_LIMIT_CFORMAT(min_interval, quill::get_root_logger(), fmt, ##__VA_ARGS__)
#endif