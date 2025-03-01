#include "foo_shared/foo_shared.h"
#include "quill_wrapper_static/quill_wrapper_static.h"

#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/backend/ThreadUtilities.h"

#include <iostream>
#include <string>
#include <string_view>

int main()
{
  setup_quill();

  LOG_INFO(get_logger("root"), "This is a log info example {}", 123);

  init_foo();

  test_foo();

  LOG_INFO(get_logger("root"), "This is a log info example {}", 456);
  LOG_INFO(get_logger("root"), "This is a log info example {}", 456);
  LOG_INFO(get_logger("root"), "This is a log info example {}", 456);
}