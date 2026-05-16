/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/Logger.h"
#include "quill/backend/BackendOptions.h"
#include "quill/sinks/ConsoleSink.h"
#include "quill/sinks/FileSink.h"

#include <memory>
#include <string>
#include <utility>

QUILL_BEGIN_NAMESPACE

namespace detail
{
inline Logger* simple_logger_impl(std::string const& output)
{
  Logger* logger = Frontend::get_logger(output);

  if (!logger)
  {
    std::shared_ptr<Sink> sink;

    if (output == "stdout" || output == "stderr")
    {
      sink = Frontend::create_or_get_sink<ConsoleSink>(output);
    }
    else
    {
      sink = Frontend::create_or_get_sink<FileSink>(output);
    }

    logger = Frontend::create_or_get_logger(
      output, std::move(sink),
      PatternFormatterOptions{
        "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(message)"});
  }

  return logger;
}
} // namespace detail

/**
 * @brief Convenience function for trivial programs and simple use cases.
 *
 * Example usage:
 * @code
 * #include "quill/SimpleSetup.h"
 * #include "quill/LogMacros.h"
 *
 * int main() {
 *   // Log to console (stdout) - defaults to stdout
 *   auto* logger = quill::simple_logger();
 *   LOG_INFO(logger, "Hello from {}!", "Quill");
 *
 *   // Or log to a file
 *   auto* logger2 = quill::simple_logger("test.log");
 *   LOG_INFO(logger2, "This message goes to a file");
 * }
 * @endcode
 *
 * @param output The output destination. Use "stdout" (default) or "stderr" for console output,
 *               or provide a filename for file output. The output name is also used as the logger name.
 *
 * @return A pointer to the created or retrieved logger instance.
 */
inline Logger* simple_logger(std::string const& output = "stdout")
{
  Logger* logger = detail::simple_logger_impl(output);

  if (!Backend::is_running())
  {
    Backend::start(BackendOptions{});
  }

  return logger;
}

/**
 * @brief Convenience function for trivial programs that also want Quill's built-in signal handler.
 *
 * This helper behaves like simple_logger() but starts the backend with the signal handler enabled.
 * If the backend is already running, the existing backend configuration is reused and no signal
 * handler changes are applied.
 *
 * @param output The output destination. Use "stdout" (default) or "stderr" for console output,
 *               or provide a filename for file output. The output name is also used as the logger name.
 * @param signal_handler_options Signal handler configuration used when the backend is started.
 *
 * @return A pointer to the created or retrieved logger instance.
 */
inline Logger* simple_logger_with_signal_handler(std::string const& output = "stdout",
                                                 SignalHandlerOptions const& signal_handler_options = SignalHandlerOptions{})
{
  Logger* logger = detail::simple_logger_impl(output);

  if (!Backend::is_running())
  {
    Backend::start<FrontendOptions>(BackendOptions{}, signal_handler_options);
  }

  return logger;
}

QUILL_END_NAMESPACE
