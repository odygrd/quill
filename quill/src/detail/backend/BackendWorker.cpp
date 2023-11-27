#include "quill/detail/backend/BackendWorker.h"
#include "quill/detail/misc/FileUtilities.h"
#include <iostream> // for endl, basic_ostream, cerr, ostream
#include <vector>   // for vector

namespace quill::detail
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
  if (!_is_running.exchange(false))
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
    std::lock_guard<std::mutex> lock{_wake_up_mutex};
    _wake_up = true;
  }

  // Signal the condition variable to wake up the worker thread
  _wake_up_cv.notify_one();
}

/***/
uint32_t BackendWorker::thread_id() const noexcept { return _backend_worker_thread_id; }

/***/
bool BackendWorker::_get_transit_event_from_queue(std::byte*& read_pos, ThreadContext* thread_context, uint64_t ts_now)
{
  // First we want to allocate a new TransitEvent or use an existing one
  // to store the message from the queue
  UnboundedTransitEventBuffer& transit_event_buffer = thread_context->transit_event_buffer();
  TransitEvent* transit_event = transit_event_buffer.back();
  transit_event->thread_id = thread_context->thread_id();
  transit_event->thread_name = thread_context->thread_name();

  // read the header first, and take copy of the header
  read_pos = align_pointer<alignof(Header), std::byte>(read_pos);
  transit_event->header = *(reinterpret_cast<Header*>(read_pos));
  read_pos += sizeof(Header);

  // if we are using rdtsc clock then here we will convert the value to nanoseconds since epoch
  // doing the conversion here ensures that every transit that is inserted in the transit buffer
  // below has a header timestamp of nanoseconds since epoch and makes it even possible to
  // have Logger objects using different clocks
  if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::Tsc)
  {
    if (!_rdtsc_clock.load(std::memory_order_relaxed))
    {
      // Here we lazy initialise rdtsc clock on the backend thread only if the user decides to use it
      // Use rdtsc clock based on config. The clock requires a few seconds to init as it is
      // taking samples first
      _rdtsc_clock.store(new RdtscClock{_rdtsc_resync_interval}, std::memory_order_release);
      _last_rdtsc_resync = std::chrono::system_clock::now();
    }

    // convert the rdtsc value to nanoseconds since epoch
    transit_event->header.timestamp =
      _rdtsc_clock.load(std::memory_order_relaxed)->time_since_epoch(transit_event->header.timestamp);

    // Now check if the message has a timestamp greater than our ts_now
    if QUILL_UNLIKELY ((ts_now != 0) && ((transit_event->header.timestamp / 1'000) >= ts_now))
    {
      // We are reading the queues sequentially and to be fair when ordering the messages
      // we are trying to avoid the situation when we already read the first queue,
      // and then we missed it when reading the last queue

      // if the message timestamp is greater than our timestamp then we stop reading this queue
      // for now and we will continue in the next circle

      // we return here and never call transit_event_buffer.push_back();
      return false;
    }
  }
  else if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::System)
  {
    if QUILL_UNLIKELY ((ts_now != 0) && ((transit_event->header.timestamp / 1'000) >= ts_now))
    {
      // We are reading the queues sequentially and to be fair when ordering the messages
      // we are trying to avoid the situation when we already read the first queue,
      // and then we missed it when reading the last queue

      // if the message timestamp is greater than our timestamp then we stop reading this queue
      // for now and we will continue in the next circle

      // we return here and never call transit_event_buffer.push_back();
      return false;
    }
  }
  else if (transit_event->header.logger_details->timestamp_clock_type() == TimestampClockType::Custom)
  {
    // we skip checking against `ts_now`, we can not compare a custom timestamp by
    // the user (TimestampClockType::Custom) against ours
  }

  // we need to check and do not try to format the flush events as that wouldn't be valid
  auto const [macro_metadata, format_fns] = transit_event->header.metadata_and_format_fn();
  auto const [format_to_fn, printf_format_to_fn] = format_fns;

  if (macro_metadata.event() != MacroMetadata::Event::Flush)
  {
#if defined(_WIN32)
    if (macro_metadata.has_wide_char())
    {
      // convert the format string to a narrow string
      size_t const size_needed = get_wide_string_encoding_size(macro_metadata.wmessage_format());
      std::string format_str(size_needed, 0);
      wide_string_to_narrow(format_str.data(), size_needed, macro_metadata.wmessage_format());

      assert(!macro_metadata.is_structured_log_template() &&
             "structured log templates are not supported for wide characters");

      auto const [pos, error] = format_to_fn(format_str, read_pos, transit_event->formatted_msg, _args);
      read_pos = pos;

      if (QUILL_UNLIKELY(!error.empty()))
      {
        // this means that fmt::format_to threw an exception, and we report it to the user
        _notification_handler(fmtquill::format("Quill ERROR: {}", error));
      }
    }
    else
    {
#endif
      if (macro_metadata.is_structured_log_template())
      {
        assert(format_to_fn &&
               "format_to_fn must be set for structured log templates, printf format is not "
               "support for structured log templates");

        // using the message_format as key for lookups
        _structured_fmt_str.assign(macro_metadata.message_format().data(),
                                   macro_metadata.message_format().size());

        std::vector<std::string> const* s_keys;

        // for messages containing named arguments threat them as structured logs
        if (auto const search = _slog_templates.find(_structured_fmt_str); search != std::cend(_slog_templates))
        {
          auto const& [fmt_str, structured_keys] = search->second;
          s_keys = &structured_keys;

          auto const [pos, error] = format_to_fn(fmt_str, read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
        else
        {
          auto [fmt_str, structured_keys] =
            _process_structured_log_template(macro_metadata.message_format());

          // insert the results
          auto res = _slog_templates.try_emplace(
            _structured_fmt_str, std::make_pair(fmt_str, std::move(structured_keys)));
          s_keys = &(res.first->second.second);

          auto const [pos, error] = format_to_fn(fmt_str, read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }

        // format the values to strings
        std::vector<std::string> structured_values;
        structured_values.reserve(s_keys->size());
        for (auto const& arg : _args)
        {
          structured_values.emplace_back(fmtquill::vformat("{}", fmtquill::basic_format_args(&arg, 1)));
        }

        // store them as kv pair
        transit_event->structured_kvs.clear();
        for (size_t i = 0; i < s_keys->size(); ++i)
        {
          transit_event->structured_kvs.emplace_back((*s_keys)[i], std::move(structured_values[i]));
        }
      }
      else
      {
        // regular logs
        if (format_to_fn)
        {
          // fmt style format
          auto const [pos, error] =
            format_to_fn(macro_metadata.message_format(), read_pos, transit_event->formatted_msg, _args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
        else
        {
          // printf style format
          auto const [pos, error] = printf_format_to_fn(macro_metadata.message_format(), read_pos,
                                                        transit_event->formatted_msg, _printf_args);

          read_pos = pos;

          if (QUILL_UNLIKELY(!error.empty()))
          {
            // this means that fmt::format_to threw an exception, and we report it to the user
            _notification_handler(fmtquill::format("Quill ERROR: {}", error));
          }
        }
      }

      if (macro_metadata.level() == LogLevel::Dynamic)
      {
        // if this is a dynamic log level we need to read the log level from the buffer
        LogLevel dynamic_log_level;
        std::memcpy(&dynamic_log_level, read_pos, sizeof(LogLevel));
        read_pos += sizeof(LogLevel);

        // Also set the dynamic log level to the transit event
        transit_event->log_level_override = dynamic_log_level;
      }
#if defined(_WIN32)
    }
#endif
  }
  else
  {
    // if this is a flush event then we do not need to format anything for the
    // transit_event, but we need to set the transit event's flush_flag pointer instead
    uintptr_t flush_flag_tmp;
    std::memcpy(&flush_flag_tmp, read_pos, sizeof(uintptr_t));
    transit_event->flush_flag = reinterpret_cast<std::atomic<bool>*>(flush_flag_tmp);
    read_pos += sizeof(uintptr_t);
  }

  // commit this transit event
  transit_event_buffer.push_back();

  return true;
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
    if (size_t const open_bracket_2_pos = fmt_template.find_first_of('{', open_bracket_pos + 1);
        open_bracket_2_pos != std::string::npos)
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
      if (size_t const close_bracket_2_pos = fmt_template.find_first_of('}', close_bracket_pos + 1);
          close_bracket_2_pos != std::string::npos)
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
void BackendWorker::_write_transit_event(TransitEvent const& transit_event) const
{
  // Forward the record to all the logger handlers
  MacroMetadata const macro_metadata = transit_event.metadata();

  for (auto& handler : transit_event.header.logger_details->handlers())
  {
    auto const& formatted_log_message_buffer = handler->formatter().format(
      std::chrono::nanoseconds{transit_event.header.timestamp}, transit_event.thread_id,
      transit_event.thread_name, _process_id, transit_event.header.logger_details->name(),
      transit_event.log_level_as_str(), macro_metadata, transit_event.formatted_msg);

    // If all filters are okay we write this message to the file
    if (handler->apply_filters(transit_event.thread_id,
                               std::chrono::nanoseconds{transit_event.header.timestamp},
                               transit_event.log_level(), macro_metadata, formatted_log_message_buffer))
    {
      // log to the handler, also pass the log_message_timestamp this is only needed in some
      // cases like daily file rotation
      handler->write(formatted_log_message_buffer, transit_event);
    }
  }
}

/***/
void BackendWorker::_process_transit_event(TransitEvent& transit_event)
{
  MacroMetadata const macro_metadata = transit_event.metadata();

  // If backend_process(...) throws we want to skip this event and move to the next, so we catch the
  // error here instead of catching it in the parent try/catch block of main_loop
  if (macro_metadata.event() == MacroMetadata::Event::Log)
  {
    if (transit_event.log_level() != LogLevel::Backtrace)
    {
      _write_transit_event(transit_event);

      // We also need to check the severity of the log message here against the backtrace
      // Check if we should also flush the backtrace messages:
      // After we forwarded the message we will check the severity of this message for this logger
      // If the severity of the message is higher than the backtrace flush severity we will also
      // flush the backtrace of the logger
      if (QUILL_UNLIKELY(transit_event.log_level() >= transit_event.header.logger_details->backtrace_flush_level()))
      {
        _backtrace_log_message_storage.process(transit_event.header.logger_details->name(),
                                               [this](TransitEvent const& te)
                                               { _write_transit_event(te); });
      }
    }
    else
    {
      // this is a backtrace log and we will store it
      _backtrace_log_message_storage.store(std::move(transit_event));
    }
  }
  else if (macro_metadata.event() == MacroMetadata::Event::InitBacktrace)
  {
    // we can just convert the capacity back to int here and use it
    _backtrace_log_message_storage.set_capacity(
      transit_event.header.logger_details->name(),
      static_cast<uint32_t>(std::stoul(
        std::string{transit_event.formatted_msg.begin(), transit_event.formatted_msg.end()})));
  }
  else if (macro_metadata.event() == MacroMetadata::Event::FlushBacktrace)
  {
    // process all records in backtrace for this logger_name and log them by calling backend_process_backtrace_log_message
    _backtrace_log_message_storage.process(transit_event.header.logger_details->name(),
                                           [this](TransitEvent const& te)
                                           { _write_transit_event(te); });
  }
  else if (macro_metadata.event() == MacroMetadata::Event::Flush)
  {
    _handler_collection.active_handlers(_active_handlers_cache);
    _force_flush();

    // this is a flush event, so we need to notify the caller to continue now
    transit_event.flush_flag->store(true);

    // we also need to reset the flush_flag as the TransitEvents are re-used
    transit_event.flush_flag = nullptr;
  }

  // Since after processing an event we never force flush but leave it up to the OS instead,
  // set this to true to keep track of unflushed messages we have
  _has_unflushed_messages = true;
}

/***/
void BackendWorker::_force_flush()
{
  if (_has_unflushed_messages)
  {
    // If we have buffered any messages then flush all active handlers
    for (auto const& handler : _active_handlers_cache)
    {
      std::shared_ptr<Handler> h = handler.lock();
      if (h)
      {
        h->flush();
      }
    }

    _has_unflushed_messages = false;
  }
}

/***/
bool BackendWorker::_check_all_queues_empty(ThreadContextCollection::backend_thread_contexts_cache_t const& cached_thread_contexts)
{
  bool all_empty{true};

  for (ThreadContext* thread_context : cached_thread_contexts)
  {
    std::visit(
      [&all_empty](auto& queue)
      {
        using T = std::decay_t<decltype(queue)>;
        if constexpr ((std::is_same_v<T, UnboundedQueue>) || (std::is_same_v<T, BoundedQueue>))
        {
          all_empty &= queue.empty();
        }
      },
      thread_context->spsc_queue_variant());
  }

  return all_empty;
}

/***/
void BackendWorker::_resync_rdtsc_clock()
{
  if (_rdtsc_clock.load(std::memory_order_relaxed))
  {
    // resync in rdtsc if we are not logging so that time_since_epoch() still works

    if (auto const now = std::chrono::system_clock::now(); (now - _last_rdtsc_resync) > _rdtsc_resync_interval)
    {
      if (_rdtsc_clock.load(std::memory_order_relaxed)->resync(2500))
      {
        _last_rdtsc_resync = now;
      }
    }
  }
}
} // namespace quill::detail