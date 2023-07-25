#include "quill/Quill.h"
#include <csignal>

int divide_by_zero()
{
  int a = 1;
  int b = 0;
  return a / b;
}

void cause_segfault()
{
  int* p = (int*)0x12345678;
  *p = 0;
}

void stack_overflow()
{
  int foo[1000];
  (void)foo;
  stack_overflow();
}

void infinite_loop()
{
  /* break out with ctrl+c to test SIGINT handling */
  while (1) {};
}

void illegal_instruction() { raise(SIGILL); }

/**
 * Signal handler example
 */

int main()
{
#if defined(_WIN32)
  // NOTE: On windows a signal handler must be installed on each new thread
  quill::init_signal_handler();
#endif

  // Start the logging backend thread
  // Passing true sets up a signal handler
  // On Linux/Macos one signal handler is set to handle POSIX style signals
  // On Windows an exception handler and a Ctrl-C handler is set.
  quill::start(true);

  quill::Logger* logger = quill::get_logger();

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // create threads that will segfault
  std::vector<std::thread> threads;
  for (size_t i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([]() {

#if defined(_WIN32)
      // NOTE: On windows the signal handler must be installed on each new thread
      quill::init_signal_handler();
#endif

      // sleep for 1 second so all threads are ready
        std::this_thread::sleep_for(std::chrono::seconds{1});

        for (size_t i = 0; i < 10; ++i)
        {
          // log 10 messages
          LOG_INFO(quill::get_logger(), "Log from thread {}", i);
        }

        LOG_INFO(quill::get_logger(), "Crash after 10 messages");

        // After 10 messages Crash - Uncomment any of the below :

        // divide_by_zero();
        // stack_overflow();
        // illegal_instruction();
        // cause_segfault();
      }));
  }

  for (uint32_t cnt{0}; cnt < 1000; ++cnt)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{300});
    LOG_INFO(quill::get_logger(), "Log from main {}", cnt);
  }
}