#include "quill/detail/backend/BackendWorker.h"
#include "quill/detail/misc/FileUtilities.h"
#include <iostream> // for endl, basic_ostream, cerr, ostream
#include <vector>   // for vector

namespace quill
{
namespace detail
{
/***/
BackendWorker::BackendWorker(Config const& config, ThreadContextCollection& thread_context_collection,
                             HandlerCollection& handler_collection, LoggerCollection& logger_collection)
  : _config(config),
    _thread_context_collection(thread_context_collection),
    _handler_collection(handler_collection),
    _logger_collection(logger_collection),
    _process_id(fmtquill::format_int(get_process_id()).str())
{
  // set up the default error handler. This is done here to avoid including std::cerr in a header file
  _notification_handler = [](std::string const& s) { std::cerr << s << std::endl; };
}

/***/
BackendWorker::~BackendWorker()
{
  // This destructor will run during static destruction as the thread is part of the singleton
  stop();
}

/***/
void BackendWorker::stop() noexcept
{
  // Stop the backend worker
  auto const is_running = _is_running.exchange(false);

  if (!is_running)
  {
    // already stopped
    return;
  }

  // signal wake up the backend worker thread
  wake_up();

  // Wait the backend thread to join, if backend thread was never started it won't be joinable so we can still
  if (_backend_worker_thread.joinable())
  {
    _backend_worker_thread.join();
  }
}

/***/
void BackendWorker::wake_up()
{
  // Set the flag to indicate that the data is ready
  {
    std::lock_guard<std::mutex> lock(_wake_up_mutex);
    _wake_up = true;
  }

  // Signal the condition variable to wake up the worker thread
  _wake_up_cv.notify_one();
}

/***/
uint32_t BackendWorker::thread_id() const noexcept { return _backend_worker_thread_id; }

/***/
std::pair<std::string, std::vector<std::string>> BackendWorker::_process_structured_log_template(std::string_view fmt_template) noexcept
{
  std::string fmt_str;
  std::vector<std::string> keys;

  size_t cur_pos = 0;

  size_t open_bracket_pos = fmt_template.find_first_of('{');
  while (open_bracket_pos != std::string::npos)
  {
    // found an open bracket
    size_t const open_bracket_2_pos = fmt_template.find_first_of('{', open_bracket_pos + 1);

    if (open_bracket_2_pos != std::string::npos)
    {
      // found another open bracket
      if ((open_bracket_2_pos - 1) == open_bracket_pos)
      {
        open_bracket_pos = fmt_template.find_first_of('{', open_bracket_2_pos + 1);
        continue;
      }
    }

    // look for the next close bracket
    size_t close_bracket_pos = fmt_template.find_first_of('}', open_bracket_pos + 1);
    while (close_bracket_pos != std::string::npos)
    {
      // found closed bracket
      size_t const close_bracket_2_pos = fmt_template.find_first_of('}', close_bracket_pos + 1);

      if (close_bracket_2_pos != std::string::npos)
      {
        // found another open bracket
        if ((close_bracket_2_pos - 1) == close_bracket_pos)
        {
          close_bracket_pos = fmt_template.find_first_of('}', close_bracket_2_pos + 1);
          continue;
        }
      }

      // construct a fmt string excluding the characters inside the brackets { }
      fmt_str += std::string{fmt_template.substr(cur_pos, open_bracket_pos - cur_pos)} + "{}";
      cur_pos = close_bracket_pos + 1;

      // also add the keys to the vector
      keys.emplace_back(fmt_template.substr(open_bracket_pos + 1, (close_bracket_pos - open_bracket_pos - 1)));

      break;
    }

    open_bracket_pos = fmt_template.find_first_of('{', close_bracket_pos);
  }

  // add anything remaining after the last bracket
  fmt_str += std::string{fmt_template.substr(cur_pos, fmt_template.length() - cur_pos)};
  return std::make_pair(fmt_str, keys);
}

/***/
void BackendWorker::_resync_rdtsc_clock()
{
  if (_rdtsc_clock.load(std::memory_order_relaxed))
  {
    // resync in rdtsc if we are not logging so that time_since_epoch() still works
    auto const now = std::chrono::system_clock::now();
    if ((now - _last_rdtsc_resync) > _rdtsc_resync_interval)
    {
      _rdtsc_clock.load(std::memory_order_relaxed)->resync(2500);
      _last_rdtsc_resync = now;
    }
  }
}
} // namespace detail
} // namespace quill