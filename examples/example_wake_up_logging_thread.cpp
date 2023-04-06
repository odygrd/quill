#include "quill/Quill.h"

#include <iostream>

/**
 * This example shows how to wake up the backend logging thread if you configure it
 * with a high sleep duration.
 * @note: It is not recommended to configure a sleep duration higher than 1 us as in this example.
 * The only use case of this is when you do not want the backend logging thread to wake up
 * periodically and consume some CPU cycles.
 */

int main()
{
  quill::Config cfg;
  cfg.backend_thread_sleep_duration = std::chrono::hours{240};

  // Start the logging backend thread
  quill::configure(cfg);
  quill::start();

  // To demonstrate the example we have to wait for the backend logging thread to start and then
  // go into sleep as there will be nothing to log
  std::this_thread::sleep_for(std::chrono::seconds{1});

  quill::Logger* logger = quill::get_logger();

  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);

  // sleep for 5 seconds, the backend logging thread will also be sleeping so no logs are displayed
  std::cout << "waiting for the logging thread..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds{5});

  LOG_INFO(logger, "This is a log info example {}", 15);
  LOG_WARNING(logger, "This is a log warning example {}", 16);
  LOG_ERROR(logger, "This is a log error example {}", 17);

  std::cout << "logging more and still waiting for the logging thread..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds{5});

  std::cout << "waking up the logging thread now" << std::endl;
  quill::wake_up_logging_thread();
  std::this_thread::sleep_for(std::chrono::seconds{5});

  LOG_INFO(logger, "Done, logging thread will always wake up and log on destruction");
}
