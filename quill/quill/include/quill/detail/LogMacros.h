/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/misc/Common.h"

#include "quill/Fmt.h"
#include "quill/Logger.h"
#include "quill/detail/misc/Macros.h"

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
 * Check in compile time the correctness of a format string
 */
template <typename S, typename... Args, typename Char = fmt::char_t<S>>
constexpr void check_format(const S& format_str, Args&&...)
{
  fmt::internal::check_format_string<std::remove_reference_t<Args>...>(format_str);
}

// Main Log Macros
// clang-format off
#define QUILL_LOGGER_CALL(logger, log_statement_level, fmt, ...) do {                                                                                \
    check_format(FMT_STRING(fmt), ##__VA_ARGS__);                                                                                                    \
    static constexpr quill::detail::StaticLogRecordInfo log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__, fmt, log_statement_level};  \
    logger->log<log_statement_level>(&log_line_info, ##__VA_ARGS__);                                                                                 \
  } while (0)
// clang-format on

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L3
  #define LOG_TRACE_L3(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)
#else
  #define LOG_TRACE_L3(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L2
  #define LOG_TRACE_L2(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)
#else
  #define LOG_TRACE_L2(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L1
  #define LOG_TRACE_L1(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)
#else
  #define LOG_TRACE_L1(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_DEBUG
  #define LOG_DEBUG(logger, fmt, ...)                                                              \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)
#else
  #define LOG_DEBUG(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_INFO
  #define LOG_INFO(logger, fmt, ...)                                                               \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)
#else
  #define LOG_INFO(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_WARNING
  #define LOG_WARNING(logger, fmt, ...)                                                            \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)
#else
  #define LOG_WARNING(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_ERROR
  #define LOG_ERROR(logger, fmt, ...)                                                              \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)
#else
  #define LOG_ERROR(logger, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
  #define LOG_CRITICAL(logger, fmt, ...)                                                           \
    QUILL_LOGGER_CALL(logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)
#else
  #define LOG_CRITICAL(logger, fmt, ...) (void)0
#endif