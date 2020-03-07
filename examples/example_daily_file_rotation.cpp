#include "quill/Quill.h"

static char const* filename = "logfile.log";

int main()
{
  // Start the backend logging thread
  quill::start();

  // The time we want to rotate. e.g 2:00
  std::chrono::hours rotation_hours{2};
  std::chrono::minutes rotation_minutes{0};

  // Create a rotating file handler with a rotation hour and minute
  quill::Handler* file_handler = quill::daily_file_handler(filename, rotation_hours, rotation_minutes);

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("daily_logger", file_handler);

  LOG_INFO(logger_bar, "Hello from {}", "daily logger");
}