/**
 * @page copyright
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/backend/ThreadUtilities.h"

#include "quill/Logger.h"
#include "quill/core/Attributes.h"
#include "quill/core/LogLevel.h"
#include "quill/core/LoggerBase.h"
#include "quill/core/LoggerManager.h"
#include "quill/core/MacroMetadata.h"
#include "quill/core/QuillError.h"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
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

  #include <windows.h>
#else
  #include <unistd.h>
#endif

QUILL_BEGIN_NAMESPACE

/**
 * Struct to hold options for the signal handler.
 */
QUILL_BEGIN_EXPORT

struct SignalHandlerOptions
{
  /**
   * List of signals that the backend should catch if with_signal_handler is enabled.
   * Enabling the built-in signal handler overrides the process handlers for these signals.
   * On Windows, Backend::start() does not consume this list; pass the desired signals to
   * init_signal_handler<TFrontendOptions>() on each thread that needs CRT signal handling.
   */
  std::vector<int> catchable_signals{SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV};

  /**
   * Defines the timeout duration in seconds for the signal handler alarm.
   * It is only available on Linux, as Windows does not support the alarm function.
   * The signal handler sets up an alarm to ensure that the process will terminate if it does not
   * complete within the specified time frame. This is particularly useful to prevent the
   * process from hanging indefinitely in case the signal handler encounters an issue.
   */
  uint32_t timeout_seconds = 20u;

  /**
   * The name of the logger instance that the signal handler will use to log errors when
   * the application crashes. The logger is accessed by the signal handler and must be
   * created by your application using Frontend::create_or_get_logger(...).
   * If this parameter is left empty, the signal handler will automatically select the first
   * valid logger it finds, excluding any loggers whose names contain substrings specified
   * in excluded_logger_substrings.
   * If a logger name is specified but not found, the signal handler will fall back to the
   * automatic selection behavior described above.
   */
  std::string logger_name;

  /**
   * List of substrings used to exclude loggers during automatic logger selection.
   * This option is only used when logger_name is empty or when the specified logger is not found.
   * The signal handler will skip any logger whose name contains any of these substrings.
   * This is useful to avoid selecting specialized loggers that write to CSV files, binary files,
   * or other non-standard formats that may not be suitable for crash reporting.
   * Default: {"__csv__"} to exclude CSV loggers.
   */
  std::vector<std::string> excluded_logger_substrings{"__csv__"};
};

QUILL_END_EXPORT

namespace detail
{
using signal_handler_t = void (*)(int);

struct SignalHandlerRestoreEntry
{
  int signal_number;
  signal_handler_t previous_handler;
};

inline void restore_signal_handler_entries(std::vector<SignalHandlerRestoreEntry> const& previous_signal_handlers) noexcept
{
  for (auto it = previous_signal_handlers.rbegin(); it != previous_signal_handlers.rend(); ++it)
  {
    std::signal(it->signal_number, it->previous_handler);
  }
}

/***/
QUILL_NODISCARD inline bool is_synchronous_fault_signal(int signal_number) noexcept
{
  return signal_number == SIGSEGV || signal_number == SIGFPE || signal_number == SIGILL
#if defined(SIGBUS)
    || signal_number == SIGBUS
#endif
#if defined(SIGTRAP)
    || signal_number == SIGTRAP
#endif
    ;
}

/***/
QUILL_NODISCARD inline char const* get_signal_description(int32_t signal_number) noexcept
{
  switch (signal_number)
  {
  case SIGABRT:
    return "SIGABRT";
  case SIGFPE:
    return "SIGFPE";
  case SIGILL:
    return "SIGILL";
  case SIGINT:
    return "SIGINT";
  case SIGSEGV:
    return "SIGSEGV";
  case SIGTERM:
    return "SIGTERM";
#if defined(SIGBUS)
  case SIGBUS:
    return "SIGBUS";
#endif
#if defined(SIGHUP)
  case SIGHUP:
    return "SIGHUP";
#endif
#if defined(SIGQUIT)
  case SIGQUIT:
    return "SIGQUIT";
#endif
#if defined(SIGTRAP)
  case SIGTRAP:
    return "SIGTRAP";
#endif
#if defined(SIGPIPE)
  case SIGPIPE:
    return "SIGPIPE";
#endif
#if defined(SIGALRM)
  case SIGALRM:
    return "SIGALRM";
#endif
#if defined(SIGUSR1)
  case SIGUSR1:
    return "SIGUSR1";
#endif
#if defined(SIGUSR2)
  case SIGUSR2:
    return "SIGUSR2";
#endif
#if defined(SIGXCPU)
  case SIGXCPU:
    return "SIGXCPU";
#endif
#if defined(SIGXFSZ)
  case SIGXFSZ:
    return "SIGXFSZ";
#endif
#if defined(SIGVTALRM)
  case SIGVTALRM:
    return "SIGVTALRM";
#endif
#if defined(SIGPROF)
  case SIGPROF:
    return "SIGPROF";
#endif
  default:
    return "UNKNOWN";
  }
}

/***/
class SignalHandlerContext
{
public:
  SignalHandlerContext(SignalHandlerContext const&) = delete;
  SignalHandlerContext& operator=(SignalHandlerContext const&) = delete;

  /***/
  QUILL_EXPORT static SignalHandlerContext& instance() noexcept
  {
    static SignalHandlerContext instance;
    return instance;
  }

  /***/
  QUILL_NODISCARD static LoggerBase* get_logger() noexcept
  {
    LoggerBase* logger_base{nullptr};

    if (!instance().logger_name.empty())
    {
      logger_base = LoggerManager::instance().get_logger(instance().logger_name);
    }

    // This also checks if the logger was found above
    if (!logger_base || !logger_base->is_valid_logger())
    {
      logger_base = LoggerManager::instance().get_valid_logger(instance().excluded_logger_substrings);
    }

    return logger_base;
  }

  std::vector<std::string> excluded_logger_substrings{};
  std::string logger_name{};
  std::atomic<int32_t> signal_number{0};
  std::atomic<uint32_t> lock{0};
  std::atomic<uint32_t> backend_thread_id{0};
  std::atomic<uint32_t> signal_handler_timeout_seconds{20};
  std::atomic<bool> should_reraise_signal{true};
  std::mutex signal_handlers_mutex;
  std::vector<int> registered_signal_handlers{};
  std::vector<SignalHandlerRestoreEntry> previous_signal_handlers{};
#if defined(_WIN32)
  LPTOP_LEVEL_EXCEPTION_FILTER previous_exception_filter{nullptr};
  void (*exception_handler_deinit_callback)() = nullptr;
  bool console_ctrl_handler_installed{false};
#endif

private:
  SignalHandlerContext() = default;
  ~SignalHandlerContext() = default;
};

#define QUILL_SIGNAL_HANDLER_LOG(logger, log_level, fmt, ...)                                      \
  do                                                                                               \
  {                                                                                                \
    if (logger->template should_log_statement<log_level>())                                        \
    {                                                                                              \
      static constexpr quill::MacroMetadata macro_metadata{                                        \
        "SignalHandler:~", "", fmt, nullptr, log_level, quill::MacroMetadata::Event::Log};         \
                                                                                                   \
      logger->template log_statement<false>(&macro_metadata, ##__VA_ARGS__);                       \
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
    detail::sleep_for_ns(24'000ull * 3'600ull * 1'000'000'000ull); // 24000 hours
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
  bool const should_reraise_signal = SignalHandlerContext::instance().should_reraise_signal.load();

  if ((backend_thread_id == 0) || (current_thread_id == backend_thread_id))
  {
    // backend worker thread is not running or the signal handler is called in the backend worker thread
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
      std::_Exit(EXIT_SUCCESS);
    }

    if (should_reraise_signal)
    {
      // for other signals expect SIGINT and SIGTERM we re-raise
      std::signal(signal_number, SIG_DFL);
      std::raise(signal_number);
    }

    // For synchronous fault signals (SIGSEGV, SIGFPE, etc.) we must not return —
    // returning re-executes the faulting instruction, causing an infinite loop.
    if (is_synchronous_fault_signal(signal_number))
    {
      std::_Exit(EXIT_FAILURE);
    }
  }
  else
  {
    // This means signal handler is running on a frontend thread, we can log and flush
    LoggerBase* logger_base = SignalHandlerContext::instance().get_logger();

    if (logger_base)
    {
      char const* const signal_desc = get_signal_description(signal_number);

      auto logger = static_cast<LoggerImpl<TFrontendOptions>*>(logger_base);
      QUILL_SIGNAL_HANDLER_LOG(logger, LogLevel::Info, "Received signal: {} (signum: {})",
                               signal_desc, signal_number);

      if (signal_number == SIGINT || signal_number == SIGTERM)
      {
        // For SIGINT and SIGTERM, we are shutting down gracefully
        // Pass `0` to avoid calling std::this_thread::sleep_for()
        logger->flush_log(0);
        std::_Exit(EXIT_SUCCESS);
      }

      if (should_reraise_signal)
      {
        QUILL_SIGNAL_HANDLER_LOG(logger, LogLevel::Critical,
                                 "Program terminated unexpectedly due to signal: {} (signum: {})",
                                 signal_desc, signal_number);

        // This is here in order to flush the above log statement
        logger->flush_log(0);

        // Reset to the default signal handler and re-raise the signal
        std::signal(signal_number, SIG_DFL);
        std::raise(signal_number);
      }
      else
      {
        logger->flush_log(0);
      }
    }

    // If we reach here it means we have no valid logger or should_reraise_signal is false.
    // For synchronous fault signals we must not return to avoid re-executing the faulting instruction.
    if (is_synchronous_fault_signal(signal_number))
    {
      std::_Exit(EXIT_FAILURE);
    }
  }
}
} // namespace detail

/**
 * Setups a signal handler to handle fatal signals.
 *
 * @note The POSIX path calls std::signal/std::raise and takes a timed alarm. The handler itself
 *       also invokes LoggerManager lookups and frontend log_statement()/flush_log() calls, which
 *       are not strictly async-signal-safe. This is intentional: the built-in handler is a
 *       best-effort crash-preservation facility, not a universal async-signal-safe logging API.
 *       See the FAQ ("Why is the signal handler best-effort?") and overview.rst for the full list
 *       of caveats, in particular the requirement that any thread which may run the handler has
 *       either already logged once, called Frontend::preallocate(), or has the handled signals
 *       blocked on that thread.
 */
#if defined(_WIN32)
namespace detail
{
/***/
inline char const* get_error_message(DWORD ex_code)
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
    LoggerBase* logger_base = SignalHandlerContext::instance().get_logger();

    if (logger_base)
    {
      auto logger = static_cast<LoggerImpl<TFrontendOptions>*>(logger_base);
      QUILL_SIGNAL_HANDLER_LOG(logger, LogLevel::Info,
                               "Program interrupted by Ctrl+C or Ctrl+Break signal");

      // Pass `0` to avoid calling std::this_thread::sleep_for()
      logger->flush_log(0);
      std::_Exit(EXIT_SUCCESS);
    }
  }

  return FALSE;
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
    LoggerBase* logger_base = SignalHandlerContext::instance().get_logger();

    if (logger_base)
    {
      auto logger = static_cast<LoggerImpl<TFrontendOptions>*>(logger_base);

      QUILL_SIGNAL_HANDLER_LOG(logger, LogLevel::Info, "Received exception: {} (Code: {})",
                               get_error_message(exception_p->ExceptionRecord->ExceptionCode),
                               exception_p->ExceptionRecord->ExceptionCode);

      QUILL_SIGNAL_HANDLER_LOG(logger, LogLevel::Critical,
                               "Program terminated unexpectedly due to exception: {} (Code: {})",
                               get_error_message(exception_p->ExceptionRecord->ExceptionCode),
                               exception_p->ExceptionRecord->ExceptionCode);

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
void deinit_exception_handler()
{
  auto& ctx = detail::SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  if (ctx.console_ctrl_handler_installed)
  {
    SetConsoleCtrlHandler(on_console_signal<TFrontendOptions>, FALSE);
    ctx.console_ctrl_handler_installed = false;
  }

  SetUnhandledExceptionFilter(ctx.previous_exception_filter);
  ctx.previous_exception_filter = nullptr;
  ctx.exception_handler_deinit_callback = nullptr;
}

/***/
template <typename TFrontendOptions>
void init_exception_handler()
{
  auto& ctx = detail::SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  ctx.exception_handler_deinit_callback = nullptr;
  ctx.previous_exception_filter = SetUnhandledExceptionFilter(on_exception<TFrontendOptions>);

  if (!SetConsoleCtrlHandler(on_console_signal<TFrontendOptions>, TRUE))
  {
    SetUnhandledExceptionFilter(ctx.previous_exception_filter);
    ctx.previous_exception_filter = nullptr;
    QUILL_THROW(QuillError{"Failed to call SetConsoleCtrlHandler"});
  }

  ctx.exception_handler_deinit_callback = &deinit_exception_handler<TFrontendOptions>;
  ctx.console_ctrl_handler_installed = true;
}
} // namespace detail

QUILL_BEGIN_EXPORT

/**
 * On windows, it has to be called on each thread
 * Do not call it from the backend worker thread; signal handling is intended for frontend/user
 * threads so the handler can safely flush through the backend.
 * @param catchable_signals the signals we are catching
 */
template <typename TFrontendOptions>
void init_signal_handler(std::vector<int> const& catchable_signals = std::vector<int>{
                           SIGTERM, SIGINT, SIGABRT, SIGFPE, SIGILL, SIGSEGV})
{
  auto& ctx = detail::SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  std::vector<detail::SignalHandlerRestoreEntry> previous_signal_handlers;
  previous_signal_handlers.reserve(catchable_signals.size());

  for (auto const& catchable_signal : catchable_signals)
  {
    // setup a signal handler per signal in the array
    auto const previous_handler = std::signal(catchable_signal, detail::on_signal<TFrontendOptions>);
    if (previous_handler == SIG_ERR)
    {
      detail::restore_signal_handler_entries(previous_signal_handlers);
      QUILL_THROW(QuillError{"Failed to setup signal handler for signal: " + std::to_string(catchable_signal)});
    }

    previous_signal_handlers.push_back({catchable_signal, previous_handler});
  }

  ctx.registered_signal_handlers = catchable_signals;
  ctx.previous_signal_handlers = std::move(previous_signal_handlers);
}

QUILL_END_EXPORT

namespace detail
{
inline void deinit_signal_handler()
{
  auto& ctx = SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  for (auto const& signal_number : ctx.registered_signal_handlers)
  {
    std::signal(signal_number, SIG_DFL);
  }

  ctx.registered_signal_handlers.clear();
  ctx.previous_signal_handlers.clear();
}

inline void restore_signal_handlers()
{
  auto& ctx = SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  restore_signal_handler_entries(ctx.previous_signal_handlers);
  ctx.registered_signal_handlers.clear();
  ctx.previous_signal_handlers.clear();
}
} // namespace detail
#else
namespace detail
{
/***/
inline void on_alarm(int32_t signal_number)
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
void init_signal_handler(std::vector<int> const& catchable_signals)
{
  auto& ctx = SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  std::vector<SignalHandlerRestoreEntry> previous_signal_handlers;
  previous_signal_handlers.reserve(catchable_signals.size() + 1);

  for (auto const& catchable_signal : catchable_signals)
  {
    if (catchable_signal == SIGALRM)
    {
      restore_signal_handler_entries(previous_signal_handlers);
      QUILL_THROW(QuillError{"SIGALRM can not be part of catchable_signals."});
    }

    // set up a signal handler per signal in the array
    auto const previous_handler = std::signal(catchable_signal, on_signal<TFrontendOptions>);
    if (previous_handler == SIG_ERR)
    {
      restore_signal_handler_entries(previous_signal_handlers);
      QUILL_THROW(QuillError{"Failed to setup signal handler for signal: " + std::to_string(catchable_signal)});
    }

    previous_signal_handlers.push_back({catchable_signal, previous_handler});
  }

  /* Register the alarm handler */
  auto const previous_alarm_handler = std::signal(SIGALRM, on_alarm);
  if (previous_alarm_handler == SIG_ERR)
  {
    restore_signal_handler_entries(previous_signal_handlers);
    QUILL_THROW(QuillError{"Failed to setup signal handler for signal: SIGALRM"});
  }

  previous_signal_handlers.push_back({SIGALRM, previous_alarm_handler});

  ctx.registered_signal_handlers = catchable_signals;
  ctx.registered_signal_handlers.push_back(SIGALRM);
  ctx.previous_signal_handlers = std::move(previous_signal_handlers);
}

inline void deinit_signal_handler()
{
  auto& ctx = SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  for (auto const& signal_number : ctx.registered_signal_handlers)
  {
    std::signal(signal_number, SIG_DFL);
  }

  ctx.registered_signal_handlers.clear();
  ctx.previous_signal_handlers.clear();
}

inline void restore_signal_handlers()
{
  auto& ctx = SignalHandlerContext::instance();
  std::lock_guard<std::mutex> const lock{ctx.signal_handlers_mutex};

  restore_signal_handler_entries(ctx.previous_signal_handlers);
  ctx.registered_signal_handlers.clear();
  ctx.previous_signal_handlers.clear();
}
} // namespace detail
#endif

QUILL_END_NAMESPACE
