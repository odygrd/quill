#include "quill/Quill.h"

static char const* base_filename = "logfile.log";

int main()
{
  // Start the backend logging thread
  quill::start();

  // Create a rotating file handler with a max file size per log file and maximum rotation up to 5 times
  std::shared_ptr<quill::Handler> file_handler =
    quill::rotating_file_handler(base_filename, "w", quill::FilenameAppend::None, 1024, 5);

  // Create a logger using this handler
  quill::Logger* logger_bar = quill::create_logger("rotating", std::move(file_handler));

  for (uint32_t i = 0; i < 15; ++i)
  {
    LOG_INFO(logger_bar, "Hello from {} {}", "rotating logger", i);
  }
}
