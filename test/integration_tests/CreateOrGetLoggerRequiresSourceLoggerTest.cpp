#include "doctest/doctest.h"

#include "quill/Frontend.h"

using namespace quill;

/***/
TEST_CASE("create_or_get_logger_requires_source_logger_to_create")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS(Frontend::create_or_get_logger("missing_logger_without_source"), QuillError);
#endif
}
