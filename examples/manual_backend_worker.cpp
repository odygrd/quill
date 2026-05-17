#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <atomic>
#include <chrono>
#include <thread>

/**
 * This example shows how to run Quill's backend worker on your own thread using
 * `ManualBackendWorker`.
 *
 * This is an advanced integration path. Prefer `Backend::start()` unless you
 * specifically need to control where and when backend polling happens.
 *
 * The example logs to the console so the output is visible immediately.
 */

int main()
{
  std::atomic<bool> backend_ready{false};
  std::atomic<bool> stop_backend{false};

  std::thread backend_thread{[&]()
                             {
                               quill::ManualBackendWorker* manual_backend_worker{
                                 quill::Backend::acquire_manual_backend_worker()};

                               quill::BackendOptions backend_options{};
                               manual_backend_worker->init(backend_options);
                               backend_ready.store(true, std::memory_order_release);

                               while (!stop_backend.load(std::memory_order_acquire))
                               {
                                 manual_backend_worker->poll(std::chrono::microseconds{50});
                                 std::this_thread::yield();
                               }

                               // Drain any remaining log messages before the thread exits.
                               manual_backend_worker->poll();
                               manual_backend_worker->shutdown();
                             }};

  while (!backend_ready.load(std::memory_order_acquire))
  {
    std::this_thread::yield();
  }

  auto console_sink{
    quill::Frontend::create_or_get_sink<quill::ConsoleSink>("manual_backend_console_sink")};

  quill::Logger* logger{quill::Frontend::create_or_get_logger("manual_backend", std::move(console_sink))};

  for (int i = 0; i < 10; ++i)
  {
    LOG_INFO(logger, "manual backend message {}", i);
  }

  stop_backend.store(true, std::memory_order_release);
  backend_thread.join();
}
