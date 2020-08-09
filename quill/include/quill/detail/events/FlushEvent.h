/**
 * Copyright(c) 2020-present, Odysseas Georgoudis & quill contributors.
 * Distributed under the MIT License (http://opensource.org/licenses/MIT)
 */

#pragma once

#include "quill/detail/events/BaseEvent.h"
#include <functional>

namespace quill
{
namespace detail
{
/**
 * Special type of event to make a caller thread wait until all log is flushed.
 * We use that to make the caller wait until the logger thread has flushed all LogRecordEvents to all handlers
 */
class FlushEvent final : public BaseEvent
{
public:
  explicit FlushEvent(std::function<void()>&& frontend_callback)
    : _frontend_callback(std::move(frontend_callback))
  {
  }

  QUILL_NODISCARD std::unique_ptr<BaseEvent> clone() const override
  {
    return std::make_unique<FlushEvent>(*this);
  }

  QUILL_NODISCARD size_t size() const noexcept override { return sizeof(*this); }

  /**
   * When we encounter this message we are going to call flush for all loggers on all handlers.
   * @param obtain_active_handlers a function that is passed to this method and obtains all the active handlers when called
   */
  void backend_process(BacktraceLogRecordStorage&, char const*,
                       GetHandlersCallbackT const& obtain_active_handlers, GetRealTsCallbackT const&) const override
  {
    std::vector<Handler*> const active_handlers = obtain_active_handlers();

    // Flush all handlers in all active handlers
    for (auto const handler : active_handlers)
    {
      handler->flush();
    }

    // Call the frontend callback this will wake up the thread that requested to flush
    _frontend_callback();
  }

  /**
   * Destructor
   */
  ~FlushEvent() override = default;

private:
  std::function<void()> _frontend_callback;
};

} // namespace detail
} // namespace quill