#include "quill/detail/SignalHandler.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "quill/Quill.h"
#include "quill/QuillError.h"
#include "quill/detail/misc/Os.h"

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <unistd.h>
#endif

namespace quill
{
namespace detail
{
namespace
{
// std::atomic is also safe here to use instead, as long as it is lock-free
volatile static std::sig_atomic_t lock{0};

#if defined(_WIN32)
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
BOOL WINAPI on_console_signal(DWORD signal)
{
  if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT)
  {
    // This means signal handler is running a caller thread, we can log from the default logger
    LOG_INFO(quill::get_logger(), "Interrupted by Ctrl+C:");

    quill::flush();
    std::exit(EXIT_SUCCESS);
  }

  return TRUE;
}

/***/
LONG WINAPI on_exception(EXCEPTION_POINTERS* exception_p)
{
  // Get the id of this thread in the handler and make sure it is not the backend worker thread
  uint32_t const tid = get_thread_id();
  if ((LogManagerSingleton::instance().log_manager().backend_worker_thread_id() == 0) ||
      (tid == LogManagerSingleton::instance().log_manager().backend_worker_thread_id()))
  {
    // backend worker thread is not running or the handler is called in the backend worker thread
  }
  else
  {
    // This means signal handler is running a caller thread, we can log from the default logger
    LOG_INFO(quill::get_logger(), "Received exception code: {}",
             get_error_message(exception_p->ExceptionRecord->ExceptionCode));

    LOG_CRITICAL(quill::get_logger(), "Terminated unexpectedly because of exception code: {}",
                 get_error_message(exception_p->ExceptionRecord->ExceptionCode));

    quill::flush();
  }

  // FATAL Exception: It doesn't necessarily stop here. we pass on continue search
  // If nobody catches it, then it will exit anyhow.
  // The RISK here is if someone is catching this and returning "EXCEPTION_EXECUTE_HANDLER"
  // but does not shutdown then the software will be running with quill shutdown.
  return EXCEPTION_CONTINUE_SEARCH;
}

/***/
void on_signal(int32_t signal_number)
{
  // This handler can be entered by multiple threads. We only allow the first thread to enter
  // the signal handler
  ++lock;
  while (lock != 1)
  {
    // sleep until a signal is delivered that either terminates the process or causes the
    // invocation of a signal-catching function.
    std::this_thread::sleep_for(std::chrono::hours{1});
  }

  // Get the id of this thread in the handler and make sure it is not the backend worker thread
  uint32_t const tid = get_thread_id();
  if ((LogManagerSingleton::instance().log_manager().backend_worker_thread_id() == 0) ||
      (tid == LogManagerSingleton::instance().log_manager().backend_worker_thread_id()))
  {
    // backend worker thread is not running or the handler is called in the backend worker thread
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
    // This means signal handler is running a caller thread, we can log from the default logger
    LOG_INFO(quill::get_logger(), "Received signal: {}", signal_number);

    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
      // For SIGINT and SIGTERM, we are shutting down gracefully
      quill::flush();
      std::exit(EXIT_SUCCESS);
    }
    else
    {
      LOG_CRITICAL(quill::get_logger(), "Terminated unexpectedly because of signal: {}", signal_number);

      quill::flush();

      // Reset to the default signal handler and re-raise the signal
      std::signal(signal_number, SIG_DFL);
      std::raise(signal_number);
    }
  }
}

#else
volatile static std::sig_atomic_t signal_number_{0};

/***/
void on_alarm(int32_t signal_number)
{
  if (signal_number_ == 0)
  {
    // Check SIGALRM is the first signal we receive
    // if SIGALRM is the first signal we ever receive then signal_number_ will be 0
    signal_number_ = signal_number;
  }

  // We will raise the original signal back
  std::signal(signal_number_, SIG_DFL);
  std::raise(signal_number_);
}

/***/
void on_signal(int32_t signal_number)
{
  // This handler can be entered by multiple threads. We only allow the first thread to enter
  // the signal handler
  lock = lock + 1;
  while (lock != 1)
  {
    // sleep until a signal is delivered that either terminates the process or causes the
    // invocation of a signal-catching function.
    pause();
  }

  // Store the original signal number
  signal_number_ = signal_number;

  // We setup an alarm to crash after 20 seconds by redelivering the original signal,
  // in case anything else goes wrong
  alarm(std::chrono::seconds{20}.count());

  // Get the id of this thread in the handler and make sure it is not the backend worker thread
  uint32_t const tid = get_thread_id();
  if ((LogManagerSingleton::instance().log_manager().backend_worker_thread_id() == 0) ||
      (tid == LogManagerSingleton::instance().log_manager().backend_worker_thread_id()))
  {
    // backend worker thread is not running or the handler is called in the backend worker thread
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
    // This means signal handler is running a caller thread, we can log from the default logger
    LOG_INFO(quill::get_logger(), "Received signal: {}", ::strsignal(signal_number));

    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
      // For SIGINT and SIGTERM, we are shutting down gracefully
      quill::flush();
      std::exit(EXIT_SUCCESS);
    }
    else
    {
      LOG_CRITICAL(quill::get_logger(), "Terminated unexpectedly because of signal: {}",
                   ::strsignal(signal_number));

      quill::flush();

      // Reset to the default signal handler and re-raise the signal
      std::signal(signal_number, SIG_DFL);
      std::raise(signal_number);
    }
  }
}
#endif
} // namespace

/***/
#if defined(_WIN32)
void init_exception_handler()
{
  SetUnhandledExceptionFilter(on_exception);

  if (!SetConsoleCtrlHandler(on_console_signal, TRUE))
  {
    QUILL_THROW(QuillError{"Failed to call SetConsoleCtrlHandler"});
  }
}

/***/
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals)
{
  for (auto const& catchable_signal : catchable_signals)
  {
    // setup a signal handler per signal in the array
    if (std::signal(catchable_signal, on_signal) == SIG_ERR)
    {
      QUILL_THROW(QuillError{"Failed to setup signal handler for signal: " + std::to_string(catchable_signal)});
    }
  }
}
#else
/***/
void init_signal_handler(std::initializer_list<int32_t> const& catchable_signals)
{
  for (auto const& catchable_signal : catchable_signals)
  {
    if (catchable_signal == SIGALRM)
    {
      QUILL_THROW(QuillError{"SIGALRM can not be part of catchable_signals."});
    }

    // setup a signal handler per signal in the array
    if (std::signal(catchable_signal, on_signal) == SIG_ERR)
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
#endif
} // namespace detail
} // namespace quill
