#include "quill/StopWatch.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <thread>

/**
 * Stopwatch logging example
 */

int main()
{
  quill::BackendOptions backend_options;
  quill::Backend::start(backend_options);

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");

  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  {
    quill::StopWatchTsc swt;
    LOG_INFO(logger, "Begin Tsc StopWatch");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG_INFO(logger, "After 1s, elapsed: {:.6}s", swt);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO(logger, "After 500ms, elapsed: {}s", swt);
    LOG_INFO(logger, "elapsed: {}", swt.elapsed_as<std::chrono::nanoseconds>());
    LOG_INFO(logger, "elapsed: {}", swt.elapsed_as<std::chrono::seconds>());
    LOG_INFO(logger, "Reset");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO(logger, "After 500ms, elapsed: {}", swt);
  }

  {
    quill::StopWatchChrono swt;
    LOG_INFO(logger, "Begin Chrono StopWatch");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG_INFO(logger, "After 1s, elapsed: {:.6}s", swt);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO(logger, "After 500ms, elapsed: {}s", swt);
    LOG_INFO(logger, "elapsed: {}", swt.elapsed_as<std::chrono::nanoseconds>());
    LOG_INFO(logger, "elapsed: {}", swt.elapsed_as<std::chrono::seconds>());
    LOG_INFO(logger, "Reset");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_INFO(logger, "After 500ms, elapsed: {}", swt);
  }
}
