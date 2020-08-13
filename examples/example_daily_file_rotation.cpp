#include "quill/Quill.h"

static char const* filename = "logfile.log";

int main()
{
  // Start the backend logging thread
  quill::start();

  // Create a rotating file handler which rotates daily at 02:00
  quill::Handler* file_handler = quill::time_rotating_file_handler(
    filename, "w", "daily", 1, 10, quill::Timezone::LocalTime, "18:30");

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("daily_logger", file_handler);

  for (int i = 0; i < 5000; ++i)
  {
    LOG_INFO(logger_bar, "Hello from {}", "daily logger");
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
}