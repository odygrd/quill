#include "quill/core/Attributes.h"

#include <new>
#include <typeinfo>
#include <utility>

import quill;

QUILL_EXPORT quill::Logger* start_module_logger()
{
  quill::Backend::start();

  auto sink =
    quill::Frontend::create_or_get_sink<quill::ConsoleSink>("module_shared_thread_id_sink");
  return quill::Frontend::create_or_get_logger(
    "module_shared_thread_id_logger", std::move(sink),
    quill::PatternFormatterOptions{"caller %(thread_id), %(message)"});
}
