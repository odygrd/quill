#include <cstdio>
#include <new>
#include <typeinfo>
#include <utility>

import quill;

#include "quill/LogMacros.h"

bool log_from_another_module_translation_unit()
{
  quill::Logger* logger = quill::Frontend::get_logger("bazel_module_logging_test_logger");
  if (!logger)
  {
    std::fprintf(stderr, "Module importers did not share the logger registry\n");
    return false;
  }

  LOG_INFO(logger, "Module logging from another translation unit: {}", 43);
  return true;
}
