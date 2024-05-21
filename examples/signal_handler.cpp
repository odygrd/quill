#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <thread>
#include <utility>
#include <vector>

void cause_segfault(quill::Logger* logger)
{
  LOG_INFO(logger, "Crashing");

  int* p = (int*)0x12345678;
  *p = 0;
}

void infinite_loop(quill::Logger* logger)
{
  LOG_INFO(logger, "Crashing");

  /* break out with ctrl+c to test SIGINT handling */
  while (1)
  {
  };
}

void illegal_instruction(quill::Logger* logger)
{
  LOG_INFO(logger, "Crashing");
  raise(SIGILL);
}

/**
 * Signal handler example
 * The signal handler flushes the log when the application crashes or gets terminated
 */

int main()
{
#if defined(_WIN32)
  // NOTE: On windows a signal handler must be installed on each new thread
  quill::init_signal_handler<quill::FrontendOptions>();
#endif

  // Start the logging backend thread with a signal handler
  // On Linux/Macos one signal handler is set to handle POSIX style signals
  // On Windows an exception handler and a Ctrl-C handler is set.
  quill::Backend::start_with_signal_handler<quill::FrontendOptions>();

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  // create threads that will cause a segfault
  std::vector<std::thread> threads;
  for (size_t i = 0; i < 4; ++i)
  {
    threads.emplace_back(
      [logger]()
      {
#if defined(_WIN32)
        // NOTE: On windows the signal handler must be installed on each new thread
        quill::init_signal_handler<quill::FrontendOptions>();
#endif
        // sleep for 1 second so all threads are ready
        std::this_thread::sleep_for(std::chrono::seconds{1});

        for (size_t i = 0; i < 10; ++i)
        {
          // log 10 messages
          LOG_INFO(logger, "Log from thread {}", i);
        }

        // After 10 messages Crash - Uncomment any of the below :
        
        // illegal_instruction(logger);
        // cause_segfault(logger);
      });
  }

  for (uint32_t cnt{0}; cnt < 1000; ++cnt)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{300});
    LOG_INFO(logger, "Log from main {}", cnt);
  }
}