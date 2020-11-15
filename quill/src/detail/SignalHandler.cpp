#include <cstdlib>
#include <cstring>

#include "quill/Quill.h"
#include "quill/QuillError.h"
#include "quill/detail/SignalHandler.h"
#include "quill/detail/misc/Os.h"

namespace
{
// std::atomic is also safe here to use instead, as long as it is lock-free
volatile static std::sig_atomic_t lock{0};
} // namespace

namespace quill
{
namespace detail
{

/***/
void on_signal(int32_t signal_number)
{
  // This handler can be entered by multiple threads. We only allow the first thread to enter
  // the signal handler
  ++lock;
  while (lock != 1)
  {
    // sleep forever
    std::this_thread::sleep_for(std::chrono::hours{1});
  }

  // Get the id of this thread in the handler and make sure it is not the backend worker thread
  uint32_t const tid = get_thread_id();
  if ((LogManagerSingleton::instance().log_manager().backend_worker_thread_id() == 0) ||
      (tid == LogManagerSingleton::instance().log_manager().backend_worker_thread_id()))
  {
    // backend worker thread is not running or the handler is called in the backend worker thread
    if (signal_number != SIGINT && signal_number != SIGTERM)
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
#if defined(_WIN32)
    LOG_INFO(quill::get_logger(), "Received signal: {}", signal_number);
#else
    LOG_INFO(quill::get_logger(), "Received signal: {}", ::strsignal(signal_number));
#endif

    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
      // For SIGINT and SIGTERM, we are shutting down gracefully
      quill::flush();
      std::exit(EXIT_SUCCESS);
    }
    else
    {
#if defined(_WIN32)
      LOG_CRITICAL(quill::get_logger(), "Terminated unexpectedly because of signal: {}", signal_number);
#else
      LOG_CRITICAL(quill::get_logger(), "Terminated unexpectedly because of signal: {}",
                   ::strsignal(signal_number));
#endif

      quill::flush();

      // Reset to the default signal handler and re-raise the signal
      std::signal(signal_number, SIG_DFL);
      std::raise(signal_number);
    }
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
} // namespace detail
} // namespace quill