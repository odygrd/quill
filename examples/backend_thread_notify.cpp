#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

/**
 * This example demonstrates how to manually wake up the backend thread when configuring
 * it with a longer sleep duration.
 *
 * Note: It's generally advised not to set the sleep duration higher than 1 second.
 * However, it's still possible to do so.
 * The only practical use case for this is when you want to prevent the backend thread
 * from waking up periodically to conserve CPU cycles.
 */

int main()
{
  // Start the backend thread
  quill::BackendOptions backend_options;
  backend_options.sleep_duration = std::chrono::hours{24};
  quill::Backend::start(backend_options);

  // To demonstrate the example we have to wait for the backend thread to start and then
  // go into sleep as there will be nothing to log, so we wait here a bit
  std::this_thread::sleep_for(std::chrono::seconds{1});

  // Frontend
  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("sink_id_1");
  quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(console_sink));

  LOG_INFO(logger, "This is a log info example {}", 5);
  LOG_WARNING(logger, "This is a log warning example {}", 6);
  LOG_ERROR(logger, "This is a log error example {}", 7);

  // sleep for 5 seconds, the backend thread will also be sleeping so no logs are displayed
  std::cout << "waiting for the backend thread..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds{5});
  std::cout << "notifying for the backend thread..." << std::endl;
  quill::Backend::notify();
  std::this_thread::sleep_for(std::chrono::seconds{1}); // let backend sleep again

  LOG_INFO(logger, "This is a log info example {}", 15);
  LOG_WARNING(logger, "This is a log warning example {}", 16);
  LOG_ERROR(logger, "This is a log error example {}", 17);

  std::cout << "waiting for the backend thread..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds{5});
  std::cout << "notifying for the backend thread..." << std::endl;
  quill::Backend::notify();
  std::this_thread::sleep_for(std::chrono::seconds{1}); // let backend sleep again

  LOG_INFO(logger, "Done, backend thread will always wake up and log on destruction");
}
