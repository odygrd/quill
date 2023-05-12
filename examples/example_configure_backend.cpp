#include "quill/Quill.h"
#include <iostream>

/**
 * Test with random configuration options for the backend thread
 * @return
 */

int main()
{
  quill::Config cfg;

  // Set a custom error handler to handler exceptions - if exceptions are enabled
  cfg.backend_thread_notification_handler = [](std::string const& s)
  { std::cout << "Hello from notification handler. Message: " << s << std::endl; };

  // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
  cfg.backend_thread_cpu_affinity = static_cast<uint16_t>(321312);

  // Set invalid thread name
  cfg.backend_thread_name =
    "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_ut_"
    "labore_et_dolore_magna_aliqua";

  // Using std::chrono clock instead of the default rdtsc clock
  cfg.default_timestamp_clock_type = quill::TimestampClockType::System;

  // apply the configuration
  quill::configure(cfg);

  // Start the logging backend thread
  quill::start();

  LOG_INFO(quill::get_logger(), "{} {}", "Hello", "World!");
}