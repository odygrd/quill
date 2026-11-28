#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/FileSink.h"

#include <unistd.h>
#include <utility>

int main()
{
  // DO NOT call Backend::start() before fork
  if (fork() == 0)
  {
    quill::Backend::start();
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("child.log");
    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
    LOG_INFO(logger, "Hello from Child {}", 123);
  }
  else
  {
    quill::Backend::start();
    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>("parent.log");
    quill::Logger* logger = quill::Frontend::create_or_get_logger("root", std::move(file_sink));
    LOG_INFO(logger, "Hello from Parent {}", 123);
  }
}
