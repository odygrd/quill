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
                             HandlerCollection const& handler_collection)
  : _config(config),
    _thread_context_collection(thread_context_collection),
    _handler_collection(handler_collection),
    _process_id(fmt::format_int(get_process_id()).str())
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // set up the default error handler
  _error_handler = [](std::string const& s) { std::cerr << s << std::endl; };
#endif
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
  _is_running.store(false, std::memory_order_relaxed);

  // Wait the backend thread to join, if backend thread was never started it won't be joinable so we can still
  if (_backend_worker_thread.joinable())
  {
    _backend_worker_thread.join();
  }
}

/***/
uint32_t BackendWorker::thread_id() const noexcept { return _backend_worker_thread_id; }

/***/
void BackendWorker::_check_dropped_messages(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts) noexcept
{
  // silence warning when bounded queue not used
  (void)cached_thread_contexts;

#if defined(QUILL_USE_BOUNDED_QUEUE)
  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    size_t const dropped_messages_cnt = thread_context->get_and_reset_message_counter();

    if (QUILL_UNLIKELY(dropped_messages_cnt > 0))
    {
      char ts[24];
      time_t t = time(nullptr);
      struct tm p;
      quill::detail::localtime_rs(std::addressof(t), std::addressof(p));
      strftime(ts, 24, "%X", std::addressof(p));

      // Write to stderr that we dropped messages
      std::string const msg = fmt::format("~ {} localtime dropped {} log messages from thread {}\n",
                                          ts, dropped_messages_cnt, thread_context->thread_id());

      detail::fwrite_fully(msg.data(), sizeof(char), msg.size(), stderr);
    }
  }
#endif
}

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

} // namespace detail
} // namespace quill