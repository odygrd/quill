#include <cstdio>
#include <new>
#include <typeinfo>
#include <utility>

import quill;

#include "quill/LogMacros.h"

quill::Logger* start_module_logger();

int main()
{
  quill::Logger* logger = start_module_logger();
  QUILL_LOG_INFO(logger, "worker {}", quill::Backend::get_thread_id());
  logger->flush_log();

  quill::Backend::stop();
  std::puts("Backend stopped");
}
