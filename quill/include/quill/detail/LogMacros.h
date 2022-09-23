/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Logger.h"
#include "quill/detail/misc/Common.h"
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
        return quill::MacroMetadata{                                                               \
          QUILL_STRINGIFY(__LINE__),       __FILE__, "n/a", fmt, log_statement_level,              \
          quill::MacroMetadata::Event::Log};                                                       \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(FMT_STRING(fmt), ##__VA_ARGS__);  \
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
        return quill::MacroMetadata{                                                               \
          QUILL_STRINGIFY(__LINE__),       __FILE__, function_name, fmt, log_statement_level,      \
          quill::MacroMetadata::Event::Log};                                                       \
      }                                                                                            \
    } anonymous_log_message_info;                                                                  \
                                                                                                   \
    if (likelyhood(logger->template should_log<log_statement_level>()))                            \
    {                                                                                              \
      logger->template log<decltype(anonymous_log_message_info)>(FMT_STRING(fmt), ##__VA_ARGS__);  \
    }                                                                                              \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ...)                                                \
  do                                                                                                 \
  {                                                                                                  \
    static constexpr char const* function_name = __FUNCTION__;                                       \
    struct                                                                                           \
    {                                                                                                \
      constexpr quill::MacroMetadata operator()() const noexcept                                     \
      {                                                                                              \
        return quill::MacroMetadata{                                                                 \
          QUILL_STRINGIFY(__LINE__),       __FILE__, function_name, fmt, quill::LogLevel::Backtrace, \
          quill::MacroMetadata::Event::Log};                                                         \
      }                                                                                              \
    } anonymous_log_message_info;                                                                    \
                                                                                                     \
    if (QUILL_LIKELY(logger->template should_log<quill::LogLevel::Backtrace>()))                     \
    {                                                                                                \
      logger->template log<decltype(anonymous_log_message_info)>(FMT_STRING(fmt), ##__VA_ARGS__);    \
    }                                                                                                \
  } while (0)

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L3
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L3(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L2
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L2(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L1
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_TRACE_L1(logger, fmt, ...) (void)0
  #define QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_DEBUG
  #define QUILL_LOG_DEBUG(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_DEBUG_NOFN(logger, fmt, ...)                                                   \
    QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_DEBUG(logger, fmt, ...) (void)0
  #define QUILL_LOG_DEBUG_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_INFO
  #define QUILL_LOG_INFO(logger, fmt, ...)                                                         \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_INFO_NOFN(logger, fmt, ...)                                                    \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_INFO(logger, fmt, ...) (void)0
  #define QUILL_LOG_INFO_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_WARNING
  #define QUILL_LOG_WARNING(logger, fmt, ...)                                                      \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_WARNING_NOFN(logger, fmt, ...)                                                 \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_WARNING(logger, fmt, ...) (void)0
  #define QUILL_LOG_WARNING_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_ERROR
  #define QUILL_LOG_ERROR(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_ERROR_NOFN(logger, fmt, ...)                                                   \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_ERROR(logger, fmt, ...) (void)0
  #define QUILL_LOG_ERROR_NOFN(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
  #define QUILL_LOG_CRITICAL(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #define QUILL_LOG_CRITICAL_NOFN(logger, fmt, ...)                                                \
    QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)
#else
  #define QUILL_LOG_CRITICAL(logger, fmt, ...) (void)0
  #define QUILL_LOG_CRITICAL_NOFN(logger, fmt, ...) (void)0
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

  #define LOG_TRACE_L3_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L3_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L2_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L2_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_TRACE_L1_NOFN(logger, fmt, ...) QUILL_LOG_TRACE_L1_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_DEBUG_NOFN(logger, fmt, ...) QUILL_LOG_DEBUG_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_INFO_NOFN(logger, fmt, ...) QUILL_LOG_INFO_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_WARNING_NOFN(logger, fmt, ...) QUILL_LOG_WARNING_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_ERROR_NOFN(logger, fmt, ...) QUILL_LOG_ERROR_NOFN(logger, fmt, ##__VA_ARGS__)
  #define LOG_CRITICAL_NOFN(logger, fmt, ...) QUILL_LOG_CRITICAL_NOFN(logger, fmt, ##__VA_ARGS__)
#endif