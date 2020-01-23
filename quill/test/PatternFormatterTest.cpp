#include <gtest/gtest.h>

#include "quill/detail/FormatPattern.h"
#include "quill/detail/PatternFormatter.h"

using namespace quill::detail;
using namespace quill;

TEST(PatternFormatter, set_pattern)
{
  PatternFormatter pattern_formatter;

  QUIL_SET_FORMAT_PATTERN(pattern_formatter,
                          "%(ascii_time) [%(thread)] %(filename):%(lineno) %(level_name) "
                          "%(logger_name) - %(message) [%(function_name)]");
}
