#include "bar.h"
#include "quill_shared.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

QUILL_EXPORT extern quill::Logger* global_logger_a;

void bar_log()
{
  std::string s{"string"};
  LOG_INFO(global_logger_a, "This is a log info example {} from bar", s);
}