/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#define QUILL_DEFINE_MACRO_METADATA(caller_function, fmt, custom_tags, log_level, is_structured_log) \
  static constexpr quill::MacroMetadata macro_metadata                                               \
  {                                                                                                  \
    __FILE__ ":" QUILL_STRINGIFY(__LINE__), caller_function, fmt, custom_tags, log_level,            \
      quill::MacroMetadata::Event::Log, is_structured_log                                            \
  }

#define QUILL_LOGGER_CALL(likelyhood, logger, log_level, fmt, ...)                                 \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log<log_level>()))                                      \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, log_level,                           \
                                  quill::detail::detect_structured_log_template(fmt));             \
                                                                                                   \
      logger->log(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                          \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_WITH_TAGS(likelyhood, logger, log_level, custom_tags, fmt, ...)          \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log<log_level>()))                                      \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, &custom_tags, log_level,                      \
                                  quill::detail::detect_structured_log_template(fmt));             \
                                                                                                   \
      logger->log(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                          \
    }                                                                                              \
  } while (0)

#define QUILL_LOGGER_CALL_LIMIT(min_interval, likelyhood, logger, log_level, fmt, ...)             \
  do                                                                                               \
  {                                                                                                \
    if (likelyhood(logger->template should_log<log_level>()))                                      \
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
    if (QUILL_LIKELY(logger->template should_log<quill::LogLevel::Backtrace>()))                   \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Backtrace,          \
                                  quill::detail::detect_structured_log_template(fmt));             \
                                                                                                   \
      logger->log(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                          \
    }                                                                                              \
  } while (0)

/**
 * Dynamic runtime log level with a tiny overhead
 * @Note: Prefer using the compile time log level macros
 */
#define QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ...)                                        \
  do                                                                                               \
  {                                                                                                \
    if (logger->should_log(log_level))                                                             \
    {                                                                                              \
      QUILL_DEFINE_MACRO_METADATA(__FUNCTION__, fmt, nullptr, quill::LogLevel::Dynamic,            \
                                  quill::detail::detect_structured_log_template(fmt));             \
                                                                                                   \
      logger->log(log_level, &macro_metadata, ##__VA_ARGS__);                                      \
    }                                                                                              \
  } while (0)

#define QUILL_DYNAMIC_LOG(logger, log_level, fmt, ...)                                             \
  QUILL_DYNAMIC_LOG_CALL(logger, log_level, fmt, ##__VA_ARGS__)

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L3
#define QUILL_LOG_TRACE_L3(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, custom_tags, fmt, ...)                              \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL3, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_TRACE_L3(logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L3_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L3_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L2
#define QUILL_LOG_TRACE_L2(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, custom_tags, fmt, ...)                              \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL2, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_TRACE_L2(logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L2_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L2_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_TRACE_L1
#define QUILL_LOG_TRACE_L1(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, fmt, ##__VA_ARGS__)

#define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, custom_tags, fmt, ...)                              \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::TraceL1, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_TRACE_L1(logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L1_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_TRACE_L1_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_DEBUG
#define QUILL_LOG_DEBUG(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

#define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_UNLIKELY, logger, quill::LogLevel::Debug, fmt, ##__VA_ARGS__)

#define QUILL_LOG_DEBUG_WITH_TAGS(logger, custom_tags, fmt, ...)                                 \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_UNLIKELY, logger, quill::LogLevel::Debug, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_DEBUG(logger, fmt, ...) (void)0
#define QUILL_LOG_DEBUG_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_DEBUG_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_INFO
#define QUILL_LOG_INFO(logger, fmt, ...)                                                         \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

#define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...)                                     \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Info, fmt, ##__VA_ARGS__)

#define QUILL_LOG_INFO_WITH_TAGS(logger, custom_tags, fmt, ...)                                  \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Info, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_INFO(logger, fmt, ...) (void)0
#define QUILL_LOG_INFO_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_INFO_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_WARNING
#define QUILL_LOG_WARNING(logger, fmt, ...)                                                      \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

#define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...)                                  \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Warning, fmt, ##__VA_ARGS__)

#define QUILL_LOG_WARNING_WITH_TAGS(logger, custom_tags, fmt, ...)                               \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Warning, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_WARNING(logger, fmt, ...) (void)0
#define QUILL_LOG_WARNING_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_WARNING_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_ERROR
#define QUILL_LOG_ERROR(logger, fmt, ...)                                                        \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

#define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...)                                    \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Error, fmt, ##__VA_ARGS__)

#define QUILL_LOG_ERROR_WITH_TAGS(logger, custom_tags, fmt, ...)                                 \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Error, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_ERROR(logger, fmt, ...) (void)0
#define QUILL_LOG_ERROR_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_ERROR_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#if QUILL_ACTIVE_LOG_LEVEL <= QUILL_LOG_LEVEL_CRITICAL
#define QUILL_LOG_CRITICAL(logger, fmt, ...)                                                     \
    QUILL_LOGGER_CALL(QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

#define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...)                                 \
    QUILL_LOGGER_CALL_LIMIT(min_interval, QUILL_LIKELY, logger, quill::LogLevel::Critical, fmt, ##__VA_ARGS__)

#define QUILL_LOG_CRITICAL_WITH_TAGS(logger, custom_tags, fmt, ...)                              \
    QUILL_LOGGER_CALL_WITH_TAGS(QUILL_LIKELY, logger, quill::LogLevel::Critical, custom_tags, fmt, ##__VA_ARGS__)
#else
#define QUILL_LOG_CRITICAL(logger, fmt, ...) (void)0
#define QUILL_LOG_CRITICAL_LIMIT(min_interval, logger, fmt, ...) (void)0
#define QUILL_LOG_CRITICAL_WITH_TAGS(logger, custom_tags, fmt, ...) (void)0
#endif

#define QUILL_LOG_BACKTRACE(logger, fmt, ...)                                                      \
  QUILL_BACKTRACE_LOGGER_CALL(logger, fmt, ##__VA_ARGS__)

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

#define LOG_TRACE_L3_WITH_TAGS(logger, custom_tags, fmt, ...)                                    \
    QUILL_LOG_TRACE_L3_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L2_WITH_TAGS(logger, custom_tags, fmt, ...)                                    \
    QUILL_LOG_TRACE_L2_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L1_WITH_TAGS(logger, custom_tags, fmt, ...)                                    \
    QUILL_LOG_TRACE_L1_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_WITH_TAGS(logger, custom_tags, fmt, ...)                                       \
    QUILL_LOG_DEBUG_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_INFO_WITH_TAGS(logger, custom_tags, fmt, ...)                                        \
    QUILL_LOG_INFO_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_WARNING_WITH_TAGS(logger, custom_tags, fmt, ...)                                     \
    QUILL_LOG_WARNING_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_ERROR_WITH_TAGS(logger, custom_tags, fmt, ...)                                       \
    QUILL_LOG_ERROR_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL_WITH_TAGS(logger, custom_tags, fmt, ...)                                    \
    QUILL_LOG_CRITICAL_WITH_TAGS(logger, custom_tags, fmt, ##__VA_ARGS__)
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

#define LOG_TRACE_L3_WITH_TAGS(custom_tags, fmt, ...)                                            \
    QUILL_LOG_TRACE_L3_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L2_WITH_TAGS(custom_tags, fmt, ...)                                            \
    QUILL_LOG_TRACE_L2_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_TRACE_L1_WITH_TAGS(custom_tags, fmt, ...)                                            \
    QUILL_LOG_TRACE_L1_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_DEBUG_WITH_TAGS(custom_tags, fmt, ...)                                               \
    QUILL_LOG_DEBUG_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_INFO_WITH_TAGS(custom_tags, fmt, ...)                                                \
    QUILL_LOG_INFO_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_WARNING_WITH_TAGS(custom_tags, fmt, ...)                                             \
    QUILL_LOG_WARNING_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_ERROR_WITH_TAGS(custom_tags, fmt, ...)                                               \
    QUILL_LOG_ERROR_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL_WITH_TAGS(custom_tags, fmt, ...)                                            \
    QUILL_LOG_CRITICAL_WITH_TAGS(quill::get_root_logger(), custom_tags, fmt, ##__VA_ARGS__)
#endif