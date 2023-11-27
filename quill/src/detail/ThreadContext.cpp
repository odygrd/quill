#include "quill/detail/ThreadContext.h"

namespace quill::detail
{
/***/
ThreadContext::ThreadContext(QueueType queue_type, uint32_t default_queue_capacity,
                             uint32_t initial_transit_event_buffer_capacity, bool huge_pages)
  : _transit_event_buffer(initial_transit_event_buffer_capacity)
{
  if ((queue_type == QueueType::UnboundedBlocking) ||
      (queue_type == QueueType::UnboundedNoMaxLimit) || (queue_type == QueueType::UnboundedDropping))
  {
    _spsc_queue.emplace<UnboundedQueue>(default_queue_capacity, huge_pages);
  }
  else
  {
    _spsc_queue.emplace<BoundedQueue>(default_queue_capacity, huge_pages);
  }
}

/***/
size_t ThreadContext::get_and_reset_message_failure_counter() noexcept
{
  if (QUILL_LIKELY(_message_failure_counter.load(std::memory_order_relaxed) == 0))
  {
    return 0;
  }
  return _message_failure_counter.exchange(0, std::memory_order_relaxed);
}
} // namespace quill::detail
