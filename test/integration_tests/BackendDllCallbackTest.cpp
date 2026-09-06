#include "doctest/doctest.h"

#include "quill/LogMacros.h"
#include "quill/Logger.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>

QUILL_EXPORT quill::Logger* start_backend_dll(void (*callback)(quill::Logger*, void*), void* context);
QUILL_EXPORT void trigger_backend_dll_callback();
QUILL_EXPORT void stop_backend_dll();

TEST_CASE("backend_dll_callback_with_immediate_flush")
{
  std::promise<void> callback_completed;
  auto completion = callback_completed.get_future();

  quill::Logger* logger = start_backend_dll(
    [](quill::Logger* callback_logger, void* context)
    {
      LOG_INFO(callback_logger, "diagnostic from the executable's backend callback");
      static_cast<std::promise<void>*>(context)->set_value();
    },
    &callback_completed);

  logger->set_immediate_flush();
  trigger_backend_dll_callback();

  bool const completed = completion.wait_for(std::chrono::seconds{5}) == std::future_status::ready;
  CHECK(completed);
  if (!completed)
  {
    // A failed implicit flush leaves the worker waiting on itself, including during normal exit.
    std::fflush(stdout);
    std::_Exit(EXIT_FAILURE);
  }

  stop_backend_dll();
}
