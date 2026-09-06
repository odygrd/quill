#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/sinks/NullSink.h"

#include <atomic>

namespace
{
std::atomic<bool> invoke_callback{false};
}

QUILL_EXPORT quill::Logger* start_backend_dll(void (*callback)(quill::Logger*, void*), void* context)
{
  auto sink = quill::Frontend::create_or_get_sink<quill::NullSink>("backend_dll_callback_sink");
  auto* logger = quill::Frontend::create_or_get_logger("backend_dll_callback_logger", std::move(sink));

  quill::BackendOptions options;
  options.backend_worker_on_poll_begin = [callback, context, logger]()
  {
    bool const should_invoke_callback = invoke_callback.exchange(false);
    if (should_invoke_callback)
    {
      callback(logger, context);
    }
  };

  quill::Backend::start(options);
  return logger;
}

QUILL_EXPORT void trigger_backend_dll_callback()
{
  invoke_callback.store(true);
  quill::Backend::notify();
}

QUILL_EXPORT void stop_backend_dll() { quill::Backend::stop(); }
