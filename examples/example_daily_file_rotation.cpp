#include "quill/Quill.h"

static char const* filename = "logfile.log";

int main()
{
  // Start the backend logging thread
  quill::start();

  // Create a rotating file handler which rotates daily at 18:30 or when the file size reaches 2GB
  std::shared_ptr<quill::Handler> file_handler =
    quill::rotating_file_handler(filename,
                                 []()
                                 {
                                   quill::RotatingFileHandlerConfig cfg;
                                   cfg.set_rotation_time_daily("18:30");
                                   cfg.set_rotation_max_file_size(2'000'000'000);
                                   return cfg;
                                 }());

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("daily_logger", std::move(file_handler));

  for (int i = 0; i < 5000; ++i)
  {
    LOG_INFO(logger_bar, "Hello from {}", "daily logger");
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
}
