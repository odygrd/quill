/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Logger.h"
#include "quill/UserClockSource.h"
#include "quill/core/Attributes.h"
#include "quill/core/Common.h"
#include "quill/core/FrontendOptions.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/PatternFormatterOptions.h"
#include "quill/core/QuillError.h"
#include "quill/core/SinkManager.h"
#include "quill/core/ThreadContextManager.h"
#include "quill/sinks/Sink.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

QUILL_BEGIN_NAMESPACE

template <typename TFrontendOptions>
class FrontendImpl
{
public:
  using frontend_options_t = TFrontendOptions;
  using logger_t = LoggerImpl<frontend_options_t>;

  /**
   * @brief Pre-allocates the thread-local data needed for the current thread.
   *
   * Although optional, it is recommended to invoke this function during the thread initialization
   * phase before the first log message.
   */
  QUILL_ATTRIBUTE_COLD static void preallocate()
  {
    uint32_t const volatile spsc_queue_capacity = detail::get_local_thread_context<TFrontendOptions>()
                                                    ->template get_spsc_queue<TFrontendOptions::queue_type>()
                                                    .capacity();

    // On windows and c++17, QUILL_MAYBE_UNUSED won't work
    (void)spsc_queue_capacity;
  }

  /**
   * @brief Creates a new sink or retrieves an existing one with the specified name.
   *
   * @param sink_name The name of the sink.
   * @param args The arguments to pass to the sink constructor.
   * @return std::shared_ptr<Sink> A shared pointer to the created or retrieved sink.
   */
  template <typename TSink, typename... Args>
  static std::shared_ptr<Sink> create_or_get_sink(std::string const& sink_name, Args&&... args)
  {
    return detail::SinkManager::instance().create_or_get_sink<TSink>(sink_name, static_cast<Args&&>(args)...);
  }

  /**
   * @brief Retrieves an existing sink with the specified name.
   *
   * @param sink_name The name of the sink.
   * @return std::shared_ptr<Sink> A shared pointer to the retrieved sink, or nullptr if not found.
   */
  QUILL_NODISCARD static std::shared_ptr<Sink> get_sink(std::string const& sink_name)
  {
    return detail::SinkManager::instance().get_sink(sink_name);
  }

  /**
   * @brief Creates a new logger or retrieves an existing one with the specified name.
   *
   * @param logger_name The name of the logger.
   * @param sink A shared pointer to the sink to associate with the logger.
   * @param pattern_formatter_options Contains the formatting configuration for PatternFormatter
   * @param clock_source The clock source for log timestamps.
   * @param user_clock A pointer to a custom user clock.
   *
   * @return Logger* A pointer to the created or retrieved logger.
   */
  static logger_t* create_or_get_logger(std::string const& logger_name, std::shared_ptr<Sink> sink,
                                        PatternFormatterOptions const& pattern_formatter_options = PatternFormatterOptions{},
                                        ClockSourceType clock_source = ClockSourceType::Tsc, UserClockSource* user_clock = nullptr)
  {
    std::vector<std::shared_ptr<Sink>> sinks;
    sinks.push_back(static_cast<std::shared_ptr<Sink>&&>(sink));

    return _cast_to_logger(detail::LoggerManager::instance().create_or_get_logger<logger_t>(
      logger_name, static_cast<std::vector<std::shared_ptr<Sink>>&&>(sinks),
      pattern_formatter_options, clock_source, user_clock));
  }

  /**
   * @brief Creates a new logger or retrieves an existing one with the specified name and multiple sinks.
   *
   * @param logger_name The name of the logger.
   * @param sinks An initializer list of shared pointers to sinks to associate with the logger.
   * @param pattern_formatter_options Contains the formatting configuration for PatternFormatter
   * @param clock_source The clock source for log timestamps.
   * @param user_clock A pointer to a custom user clock.
   * @return Logger* A pointer to the created or retrieved logger.
   */
  static logger_t* create_or_get_logger(
    std::string const& logger_name, std::initializer_list<std::shared_ptr<Sink>> sinks,
    PatternFormatterOptions const& pattern_formatter_options = PatternFormatterOptions{},
    ClockSourceType clock_source = ClockSourceType::Tsc, UserClockSource* user_clock = nullptr)
  {
    return _cast_to_logger(detail::LoggerManager::instance().create_or_get_logger<logger_t>(
      logger_name, std::vector<std::shared_ptr<Sink>>{sinks}, pattern_formatter_options, clock_source, user_clock));
  }

  /**
   * @brief Asynchronously removes the specified logger.
   * When a logger is removed, any files associated with its sinks are also closed.
   *
   * @note Thread-safe
   * @param logger A pointer to the logger to remove.
   */
  static void remove_logger(detail::LoggerBase* logger)
  {
    detail::LoggerManager::instance().remove_logger(logger);
  }

  /**
   * @brief Retrieves an existing logger with the specified name.
   *
   * @param logger_name The name of the logger.
   * @return Logger* A pointer to the retrieved logger, or nullptr if not found.
   */
  QUILL_NODISCARD static logger_t* get_logger(std::string const& logger_name)
  {
    detail::LoggerBase* logger = detail::LoggerManager::instance().get_logger(logger_name);
    return logger ? _cast_to_logger(logger) : nullptr;
  }

  /**
   * @brief Retrieves a map of all registered valid loggers.
   * @note If `remove_logger()` is called from this or another thread, the return value
   *       of this function will become invalid.
   * @return A vector containing all registered loggers.
   */
  QUILL_NODISCARD static std::vector<logger_t*> get_all_loggers()
  {
    std::vector<detail::LoggerBase*> logger_bases = detail::LoggerManager::instance().get_all_loggers();

    std::vector<logger_t*> loggers;

    for (auto const& logger_base : logger_bases)
    {
      loggers.push_back(_cast_to_logger(logger_base));
    }

    return loggers;
  }

  /**
   * Returns the first valid logger that is found. This is useful when you do not want to use the
   * std::vector<logger_t*> return value of get_all_loggers.
   *
   * @return A pointer to the first valid logger, or nullptr if no valid logger is found.
   */
  QUILL_NODISCARD static logger_t* get_valid_logger() noexcept
  {
    detail::LoggerBase* logger = detail::LoggerManager::instance().get_valid_logger();
    return logger ? _cast_to_logger(logger) : nullptr;
  }

  /**
   * @brief Counts the number of existing loggers, including any invalidated loggers.
   * This function can be useful for verifying if a logger has been removed after calling
   * remove_logger() by the backend, as removal occurs asynchronously.
   * @return The number of loggers.
   */
  QUILL_NODISCARD static size_t get_number_of_loggers() noexcept
  {
    return detail::LoggerManager::instance().get_number_of_loggers();
  }

private:
  QUILL_NODISCARD static logger_t* _cast_to_logger(detail::LoggerBase* logger_base)
  {
    assert(logger_base);

    // MSVC can complain that another variable with the same name hides this variable
    auto* _logger_cast = dynamic_cast<logger_t*>(logger_base);

    if (QUILL_UNLIKELY(!_logger_cast))
    {
      QUILL_THROW(QuillError{"Failed to cast logger. Invalid logger type."});
    }

    return _logger_cast;
  }
};

using Frontend = FrontendImpl<FrontendOptions>;

QUILL_END_NAMESPACE
