#include "quill/Quill.h"
#include <csignal>

/**
 * Signal handler example
 */

int main()
{
  // Start the logging backend thread
  quill::start(true);

  quill::Logger* logger = quill::get_logger();

  // Change the LogLevel to print everything
  logger->set_log_level(quill::LogLevel::TraceL3);

  // create threads that will segfault
  std::vector<std::thread> threads;
  for (size_t i = 0; i < 4; ++i)
  {
    threads.emplace_back(std::thread([]() {
      // sleep for 1 second so all threads are ready
      std::this_thread::sleep_for(std::chrono::seconds{1});

      for (size_t i = 0; i < 10; ++i)
      {
        // log 10 messages
        LOG_INFO(quill::get_logger(), "Log from thread {}", i);
      }

      // crash after 10 messages
      LOG_INFO(quill::get_logger(), "Crash after 10 messages");
      std::raise(SIGSEGV);
    }));
  }

  uint32_t cnt{0};
  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    LOG_INFO(quill::get_logger(), "Log from main {}", cnt++);
  }
}