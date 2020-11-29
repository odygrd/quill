/**
 * Using std::chrono clock instead of the default rdtsc clock
 */
#define QUILL_CHRONO_CLOCK

#include "quill/Quill.h"
#include <iostream>

/**
 * Test with random configuration options for the backend thread
 * @return
 */

int main()
{
#if !defined(QUILL_NO_EXCEPTIONS)
  // Set a custom error handler to handler exceptions - if exceptions are enabled
  quill::set_backend_worker_error_handler([](std::string const& s) {
    std::cout << "Hello from error handler. Error: " << s << std::endl;
  });
#endif

  // Setting to an invalid CPU. When we call quill::start() our error handler will be invoked and an error will be logged
  quill::config::set_backend_thread_cpu_affinity(static_cast<uint16_t>(321312));

  // Set invalid thread name
  quill::config::set_backend_thread_name(
    "Lorem_ipsum_dolor_sit_amet_consectetur_adipiscing_elit_sed_do_eiusmod_tempor_incididunt_ut_"
    "labore_et_dolore_magna_aliqua");

  // Start the logging backend thread
  quill::start();

  LOG_INFO(quill::get_logger(), "{} {}", "Hello", "World!");
}