/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/core/LogLevel.h"
#include "quill/core/MacroMetadata.h"

/**
 * Allows compile-time filtering of log messages to completely compile out log levels,
 * resulting in zero-cost logging.
 *
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements,
 * reducing branches in compiled code and the number of MacroMetadata constexpr instances.
 *
 * The default value of -1 enables all log levels.
 * Specify a logging level to disable all levels equal to or higher than the specified level.
 *
 * For example to only log warnings and above you can use:
 *   add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 * or
 *   target_compile_definitions(${TARGET} PRIVATE -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 **/
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3 0
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L2 1
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1 2
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG 3
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO 4
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING 5
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR 6
#define QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL 7

#if !defined(QUILL_COMPILE_ACTIVE_LOG_LEVEL)
  #define QUILL_COMPILE_ACTIVE_LOG_LEVEL -1
#endif

#define QUILL_DEFINE_MACRO_METADATA(caller_function, fmt, tags, log_level)                         \
  static constexpr quill::MacroMetadata macro_metadata                                             \
  {                                                                                                \
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), caller_function, fmt, tags, log_level,                 \
      quill::MacroMetadata::Event::Log                                                             \
  }

#define QUILL_LOGGER_CALL(likelyhood, logger, log_level, fmt, ...)                                 \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log_message<log_level>()))                              \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, log_level);                          \
                                                                                                   \
      logger->log_message(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                  \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_WITH_TAGS(likelyhood, logger, log_level, tags, fmt, ...)                 \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log_message<log_level>()))                              \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, &tags, log_level);                            \
                                                                                                   \
      logger->log_message(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                  \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_LIMIT(min_interval, likelyhood, logger, log_level, fmt, ...)             \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log_message<log_level>()))                              \
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
      QUILL_LOGGER_CALL(likelyhood, logger, log_level, fmt, ##__VA_ARGS__);                        \
    }                                                                                              \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ...)                                              \
  do                                                                                               \
  {                                                                                                \
    if (QUILL_LIKELY(logger->template should_log_message<quill::LogLevel::Backtrace>()))           \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Backtrace);         \
                                                                                                   \
      logger->log_message(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                  \
    }                                                                                              \
  } while (0)

/**
 * Dynamic runtime log level with a tiny overhead
 * @Note: Prefer using the compile time log level macros
 */
#define QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ...)                                        \
  do                                                                                               \
  {                                                                                                \
    if (logger->should_log_message(log_level))                                                     \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Dynamic);           \
                                                                                                   \
      logger->log_message(log_level, &macro_metadata, ##__VA_ARGS__);                              \
    }                                                                                              \
  } while (0)

#define QUILL_DYNAMIC_LOG(logger, log_level, fmt, ...)                                             \
  QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ##__VA_ARGS__)

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L2
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L1
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_DEBUG
  #define QUILL_LOG_DEBUG(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...)                                        \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_DEBUG(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_INFO
  #define QUILL_LOG_INFO(logger, fmt, ...)                                                         \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ...)                                         \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Info, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_INFO(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING
  #define QUILL_LOG_WARNING(logger, fmt, ...)                                                      \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...)                                      \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Warning, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_WARNING(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_ERROR
  #define QUILL_LOG_ERROR(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...)                                        \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Error, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_ERROR(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#if QUILL_COMPILE_ACTIVE_LOG_LEVEL <= QUILL_COMPILE_ACTIVE_LOG_LEVEL_CRITICAL
  #define QUILL_LOG_CRITICAL(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...)                                     \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Critical, tags, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_CRITICAL(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...) (void)0
#endif

#define QUILL_LOG_BACKTRACE(logger, fmt, ...)                                                      \
  QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ##__VA_ARGS__)

#if !defined(QUILL_DISABLE_NON_PREFIXED_MACROS)
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

  #define LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L3_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L2_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_TRACE_L1_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ...)                                              \
    QUILL_LOG_DEBUG_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_INFO_WITH_TAGS(logger, tags, fmt, ...)                                               \
    QUILL_LOG_INFO_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_WITH_TAGS(logger, tags, fmt, ...)                                            \
    QUILL_LOG_WARNING_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_WITH_TAGS(logger, tags, fmt, ...)                                              \
    QUILL_LOG_ERROR_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ...)                                           \
    QUILL_LOG_CRITICAL_WITH_TAGS(logger, tags, fmt, ##__VA_ARGS__)
#endif