#include "foo_shared.h"

#include "quill_wrapper_static/quill_wrapper_static.h"

#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

void init_foo() { setup_quill(); }

void test_foo()
{
  LOG_INFO(get_logger("root"), "{} {}", 123, 234);
}