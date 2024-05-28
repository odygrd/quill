/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/Logger.h"
#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/QuillError.h"
#include "quill/core/ThreadUtilities.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#if defined(_WIN32)
  #if !defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif

  #if !defined(NOMINMAX)
    // Mingw already defines this, so no need to redefine
    #define NOMINMAX
  #endif

  #include <chrono>
  #include <thread>
  #include <windows.h>
#else
  #include <unistd.h>
#endif

namespace quill
{
namespace detail
{
/***/
class SignalHandlerContext
{
public:
  SignalHandlerContext(SignalHandlerContext const&) = delete;
  SignalHandlerContext& operator=(SignalHandlerContext const&) = delete;

  /***/
  static SignalHandlerContext& instance() noexcept
  {
    static SignalHandlerContext instance;
    return instance;
  }

  std::atomic<int32_t> signal_number{0};
  std::atomic<uint32_t> lock{0};
  std::atomic<uint32_t> backend_thread_id{0};
  std::atomic<uint32_t> signal_handler_timeout_seconds{20};

private:
  SignalHandlerContext() = default;
  ~SignalHandlerContext() = default;
};

#define QUILL_SIGNAL_HANDLER_LOG(logger, log_level, fmt, ...)                                      \
  do                                                                                               \
  {                                                                                                \
    if (logger->template should_log_message<log_level>())                                          \
    {                                                                                              \
      static constexpr quill::MacroMetadata macro_metadata{                                        \
        "SignalHandler:~", "", fmt, nullptr, log_level, quill::MacroMetadata::Event::Log};         \
                                                                                                   \
      logger->log_message(quill::LogLevel::None, &macro_metadata, ##__VA_ARGS__);                  \
    }                                                                                              \
  } while (0)

/***/
template <typename TFrontendOptions>
void on_signal(int32_t signal_number)
{
  // This handler can be entered by multiple threads.
  uint32_t const lock = SignalHandlerContext::instance().lock.fetch_add(1);

  if (lock != 0)
  {
    // We only allow the first thread to enter the signal handler

    // sleep until a signal is delivered that either terminates the process or causes the
    // invocation of a signal-catching function.

#if defined(_WIN32)
    std::this_thread::sleep_for(std::chrono::hours{24000});
#else
    pause();
#endif
  }

#if defined(_WIN32)
  // nothing to do, windows do not have alarm
#else
  // Store the original signal number for the alarm
  SignalHandlerContext::instance().signal_number.store(signal_number);

  // Set up an alarm to crash after 20 seconds by redelivering the original signal,
  // in case anything else goes wrong
  alarm(SignalHandlerContext::instance().signal_handler_timeout_seconds.load());
#endif

  // Get the id of this thread in the handler and make sure it is not the backend worker thread
  uint32_t const backend_thread_id = SignalHandlerContext::instance().backend_thread_id.load();
  uint32_t const current_thread_id = get_thread_id();

  if ((backend_thread_id == 0) || (current_thread_id == backend_thread_id))
  {
    // backend worker thread is not running or the signal handler is called in the backend worker thread
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
      std::exit(EXIT_SUCCESS);
    }
    else
    {
      // for other signals expect SIGINT and SIGTERM we re-raise
      std::signal(signal_number, SIG_DFL);
      std::raise(signal_number);
    }
  }
  else
  {
    // This means signal handler is running on a frontend thread, we can log and flush
    std::vector<LoggerBase*> loggers = detail::LoggerManager::instance().get_all_loggers();

    if (!loggers.empty())
    {
#if defined(_WIN32)
      std::string const signal_desc = std::to_string(signal_number);
#else
      std::string const signal_desc = ::strsignal(signal_number);
#endif

      auto logger = reinterpret_cast<LoggerImpl<TFrontendOptions>*>(loggers.front());
      QUILL_SIGNAL_HANDLER_LOG(logger, quill::LogLevel::Info, "Received signal: {}", signal_desc);

      if (signal_number == SIGINT || signal_number == SIGTERM)
      {
        // For SIGINT and SIGTERM, we are shutting down gracefully
        // Pass `0` to avoid calling std::this_thread::sleep_for()
        logger->flush_log(0);
        std::exit(EXIT_SUCCESS);
      }
      else
      {
        QUILL_SIGNAL_HANDLER_LOG(logger, quill::LogLevel::Critical,
                                 "Terminated unexpectedly because of signal: {}", signal_desc);

        // Pass `0` to avoid calling std::this_thread::sleep_for()
        logger->flush_log(0);

        // Reset to the default signal handler and re-raise the signal
        std::signal(signal_number, SIG_DFL);
        std::raise(signal_number);
      }
    }
  }
}
} // namespace detail

/**
 * Setups a signal handler to handle fatal signals
 */
#if defined(_WIN32)
namespace detail
{
/***/
char const* get_error_message(DWORD ex_code)
{
  switch (ex_code)
  {
  case EXCEPTION_ACCESS_VIOLATION:
    return "EXCEPTION_ACCESS_VIOLATION";
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
  case EXCEPTION_BREAKPOINT:
    return "EXCEPTION_BREAKPOINT";
  case EXCEPTION_DATATYPE_MISALIGNMENT:
    return "EXCEPTION_DATATYPE_MISALIGNMENT";
  case EXCEPTION_FLT_DENORMAL_OPERAND:
    return "EXCEPTION_FLT_DENORMAL_OPERAND";
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
  case EXCEPTION_FLT_INEXACT_RESULT:
    return "EXCEPTION_FLT_INEXACT_RESULT";
  case EXCEPTION_FLT_INVALID_OPERATION:
    return "EXCEPTION_FLT_INVALID_OPERATION";
  case EXCEPTION_FLT_OVERFLOW:
    return "EXCEPTION_FLT_OVERFLOW";
  case EXCEPTION_FLT_STACK_CHECK:
    return "EXCEPTION_FLT_STACK_CHECK";
  case EXCEPTION_FLT_UNDERFLOW:
    return "EXCEPTION_FLT_UNDERFLOW";
  case EXCEPTION_ILLEGAL_INSTRUCTION:
    return "EXCEPTION_ILLEGAL_INSTRUCTION";
  case EXCEPTION_IN_PAGE_ERROR:
    return "EXCEPTION_IN_PAGE_ERROR";
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    return "EXCEPTION_INT_DIVIDE_BY_ZERO";
  case EXCEPTION_INT_OVERFLOW:
    return "EXCEPTION_INT_OVERFLOW";
  case EXCEPTION_INVALID_DISPOSITION:
    return "EXCEPTION_INVALID_DISPOSITION";
  case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
  case EXCEPTION_PRIV_INSTRUCTION:
    return "EXCEPTION_PRIV_INSTRUCTION";
  case EXCEPTION_SINGLE_STEP:
    return "EXCEPTION_SINGLE_STEP";
  case EXCEPTION_STACK_OVERFLOW:
    return "EXCEPTION_STACK_OVERFLOW";
  default:
    return "Unrecognized Exception";
  }
}

/***/
template <typename TFrontendOptions>
BOOL WINAPI on_console_signal(DWORD signal)
{
  uint32_t const backend_thread_id = SignalHandlerContext::instance().backend_thread_id.load();
  uint32_t const current_thread_id = get_thread_id();

  // Check if the signal handler is running from a caller thread and if the signal is CTRL+C or CTRL+BREAK
  if ((backend_thread_id != 0) && (current_thread_id != backend_thread_id) &&
      (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT))
  {
    // Log the interruption and flush log messages
    std::vector<LoggerBase*> loggers = detail::LoggerManager::instance().get_all_loggers();
    if (!loggers.empty())
    {
      auto logger = reinterpret_cast<LoggerImpl<TFrontendOptions>*>(loggers.front());
      QUILL_SIGNAL_HANDLER_LOG(logger, quill::LogLevel::Info, "Interrupted by Ctrl+C:");

      // Pass `0` to avoid calling std::this_thread::sleep_for()
      logger->flush_log(0);
      std::exit(EXIT_SUCCESS);
    }
  }

  return TRUE;
}

/***/
template <typename TFrontendOptions>
LONG WINAPI on_exception(EXCEPTION_POINTERS* exception_p)
{
  uint32_t const backend_thread_id = SignalHandlerContext::instance().backend_thread_id.load();
  uint32_t const current_thread_id = get_thread_id();

  // Check if the signal handler is running from a caller thread and if the signal is CTRL+C or CTRL+BREAK
  if ((backend_thread_id != 0) && (current_thread_id != backend_thread_id))
  {
    // Log the interruption and flush log messages
    std::vector<LoggerBase*> loggers = detail::LoggerManager::instance().get_all_loggers();
    if (!loggers.empty())
    {
      auto logger = reinterpret_cast<LoggerImpl<TFrontendOptions>*>(loggers.front());

      QUILL_SIGNAL_HANDLER_LOG(logger, quill::LogLevel::Info, "Received exception code: {}",
                               get_error_message(exception_p->ExceptionRecord->ExceptionCode));

      QUILL_SIGNAL_HANDLER_LOG(logger, quill::LogLevel::Critical,
                               "Terminated unexpectedly because of exception code: {}",
                               get_error_message(exception_p->ExceptionRecord->ExceptionCode));

      // Pass `0` to avoid calling std::this_thread::sleep_for()
      logger->flush_log(0);
    }
  }

  // FATAL Exception: It doesn't necessarily stop here. we pass on continue search
  // If nobody catches it, then it will exit anyhow.
  // The risk here is if someone is catching this and returning "EXCEPTION_EXECUTE_HANDLER"
  // but won't shut down then the app will be running with quill shutdown.
  return EXCEPTION_CONTINUE_SEARCH;
}

/***/
template <typename TFrontendOptions>
void init_exception_handler()
{
  SetUnhandledExceptionFilter(on_exception<TFrontendOptions>);

  if (!SetConsoleCtrlHandler(on_console_signal<TFrontendOptions>, TRUE))
  {
    QUILL_THROW(QuillError{"Failed to call SetConsoleCtrlHandler"});
  }
}
} // namespace detail

/**
 * On windows it has to be called on each thread
 * @param catchable_signals the signals we are catching
 */
template <typename TFrontendOptions>
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals = std::initializer_list<int>{
                           SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV})
{
  for (auto const& catchable_signal : catchable_signals)
  {
    // setup a signal handler per signal in the array
    if (std::signal(catchable_signal, detail::on_signal<TFrontendOptions>) == SIG_ERR)
    {
      QUILL_THROW(QuillError{"Failed to setup signal handler for signal: " + std::to_string(catchable_signal)});
    }
  }
}
#else
namespace detail
{
/***/
void on_alarm(int32_t signal_number)
{
  if (SignalHandlerContext::instance().signal_number.load() == 0)
  {
    // Will only happen if SIGALRM is the first signal we receive
    SignalHandlerContext::instance().signal_number = signal_number;
  }

  // We will raise the original signal back
  std::signal(SignalHandlerContext::instance().signal_number, SIG_DFL);
  std::raise(SignalHandlerContext::instance().signal_number);
}

template <typename TFrontendOptions>
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals)
{
  for (auto const& catchable_signal : catchable_signals)
  {
    if (catchable_signal == SIGALRM)
    {
      QUILL_THROW(QuillError{"SIGALRM can not be part of catchable_signals."});
    }

    // set up a signal handler per signal in the array
    if (std::signal(catchable_signal, on_signal<TFrontendOptions>) == SIG_ERR)
    {
      QUILL_THROW(QuillError{"Failed to setup signal handler for signal: " + std::to_string(catchable_signal)});
    }
  }

  /* Register the alarm handler */
  if (std::signal(SIGALRM, on_alarm) == SIG_ERR)
  {
    QUILL_THROW(QuillError{"Failed to setup signal handler for signal: SIGALRM"});
  }
}
} // namespace detail
#endif

} // namespace quill