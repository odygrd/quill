#pragma once

#include "quill/Logger.h"
#include "quill/detail/Macros.h"

// clang-format off
#define QUILL_LOG(logger, log_statement_level, fmt, ...) do {                                                               \
    static constexpr quill::detail::LogLineInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, fmt, log_statement_level};  \
    logger->log<log_statement_level>(&log_line_info, ##__VA_ARGS__);                                                        \
  } while (0)
// clang-format on

#if defined(QUILL_REDUCED_LOGGING_ENABLED)
#define LOG_TRACE_L3(logger, fmt, ...)
#define LOG_TRACE_L2(logger, fmt, ...)
#define LOG_TRACE_L1(logger, fmt, ...)
#else
#define LOG_TRACE_L3(logger, fmt, ...)                                                             \
  QUILL_LOG(logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L2(logger, fmt, ...)                                                             \
  QUILL_LOG(logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L1(logger, fmt, ...)                                                             \
  QUILL_LOG(logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)
#endif

#define LOG_DEBUG(logger, fmt, ...) QUILL_LOG(logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(logger, fmt, ...) QUILL_LOG(logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_WARNING(logger, fmt, ...)                                                              \
  QUILL_LOG(logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)
#define LOG_ERROR(logger, fmt, ...) QUILL_LOG(logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(logger, fmt, ...)                                                             \
  QUILL_LOG(logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)
