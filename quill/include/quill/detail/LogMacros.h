/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"

#include "quill/Fmt.h"
#include "quill/Logger.h"
#include "quill/detail/misc/Macros.h"
#include <type_traits>

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

/**
 * A macro to indicate that a user define type is copy_loggable
 */
#define QUILL_COPY_LOGGABLE using copy_loggable = std::true_type

/**
 * Check in compile time the correctness of a format string
 */
template <typename S, typename... Args, typename Char = fmt::char_t<S>>
constexpr void check_format(const S& format_str, Args&&...)
{
#if FMT_VERSION >= 70000
  fmt::detail::check_format_string<std::remove_reference_t<Args>...>(format_str);
#else
  fmt::internal::check_format_string<std::remove_reference_t<Args>...>(format_str);
#endif
}

// Main Log Macros

#if defined(QUILL_NOFN_MACROS)
// clang-format off
#define QUILL_LOGGER_CALL_NOFN(likelyhood, logger, log_statement_level, fmt, ...) do {                                                                    \
    check_format(FMT_STRING(fmt), ##__VA_ARGS__);                                                                                                    \
                                                                                                                                                     \
    struct {                                                                                                                                         \
      constexpr quill::detail::LogRecordMetadata operator()() const noexcept {                                                                       \
        return quill::detail::LogRecordMetadata{QUILL_STRINGIFY(__LINE__), __FILE__, "n/a", fmt, log_statement_level}; }                     \
      } anonymous_log_record_info;                                                                                                                   \
                                                                                                                                                     \
    if (likelyhood(logger->should_log<log_statement_level>()))                                                                                       \
    {                                                                                                                                                \
      logger->log<decltype(anonymous_log_record_info)>(__VA_ARGS__);                                                                                 \
    }                                                                                                                                                \
  } while (0)
// clang-format on
#endif

// clang-format off
#define QUILL_LOGGER_CALL(likelyhood, logger, log_statement_level, fmt, ...) \
  do {                                                                       \
    check_format(FMT_STRING(fmt), ##__VA_ARGS__);                                                                                                    \
                                                                                                                                                     \
    static constexpr char const* function_name = __FUNCTION__;                                                                                       \
    struct {                                                                                                                                         \
      constexpr quill::detail::LogRecordMetadata operator()() const noexcept {                                                                       \
        return quill::detail::LogRecordMetadata{QUILL_STRINGIFY(__LINE__), __FILE__, function_name, fmt, log_statement_level}; }                     \
      } anonymous_log_record_info;                                                                                                                   \
                                                                                                                                                     \
    if (likelyhood(logger->should_log<log_statement_level>()))                                                                                       \
    {                                                                                                                                                \
      constexpr bool is_backtrace_log_record {false};                                                                                                    \
      logger->log<is_backtrace_log_record, decltype(anonymous_log_record_info)>(__VA_ARGS__);                                                            \
    }                                                                                                                                                \
  } while (0)

#define QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ...) \
  do {                                                                       \
    check_format(FMT_STRING(fmt), ##__VA_ARGS__);                                                                                                    \
                                                                                                                                                     \
    static constexpr char const* function_name = __FUNCTION__;                                                                                       \
    struct {                                                                                                                                         \
      constexpr quill::detail::LogRecordMetadata operator()() const noexcept {                                                                       \
        return quill::detail::LogRecordMetadata{QUILL_STRINGIFY(__LINE__), __FILE__, function_name, fmt, quill::LogLevel::Backtrace}; }              \
      } anonymous_log_record_info;                                                                                                                   \
                                                                                                                                                     \
    if (QUILL_LIKELY(logger->should_log<quill::LogLevel::Backtrace>()))                                                                              \
    {                                                                                                                                                \
      constexpr bool is_backtrace_log_record {true};                                                                                                    \
      logger->log<is_backtrace_log_record, decltype(anonymous_log_record_info)>(__VA_ARGS__);                                                 \
    }                                                                                                                                                \
  } while (0)
// clang-format on

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L3
  #define LOG_TRACE_L3(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L3_NOFN(logger, fmt, ...)                                                    \
      QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_TRACE_L3(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L3_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L2
  #define LOG_TRACE_L2(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L2_NOFN(logger, fmt, ...)                                                    \
      QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_TRACE_L2(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L2_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L1
  #define LOG_TRACE_L1(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L1_NOFN(logger, fmt, ...)                                                    \
      QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_TRACE_L1(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_TRACE_L1_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_DEBUG
  #define LOG_DEBUG(logger, fmt, ...)                                                              \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_DEBUG_NOFN(logger, fmt, ...)                                                       \
      QUILL_LOGGER_CALL_NOFN(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_DEBUG(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_DEBUG_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_INFO
  #define LOG_INFO(logger, fmt, ...)                                                               \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_INFO_NOFN(logger, fmt, ...)                                                        \
      QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_INFO(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_INFO_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_WARNING
  #define LOG_WARNING(logger, fmt, ...)                                                            \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_WARNING_NOFN(logger, fmt, ...)                                                     \
      QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_WARNING(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_WARNING_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_ERROR
  #define LOG_ERROR(logger, fmt, ...)                                                              \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_ERROR_NOFN(logger, fmt, ...)                                                       \
      QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_ERROR(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_ERROR_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
  #define LOG_CRITICAL(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_CRITICAL_NOFN(logger, fmt, ...)                                                    \
      QUILL_LOGGER_CALL_NOFN(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)
  #endif
#else
  #define LOG_CRITICAL(logger, fmt, ...) (void)0

  #if defined(QUILL_NOFN_MACROS)
    #define LOG_CRITICAL_NOFN(logger, fmt, ...) (void)0
  #endif
#endif

#define LOG_BACKTRACE(logger, fmt, ...) QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ##__VA_ARGS__)