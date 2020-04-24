#include "quill/detail/HandlerCollection.h"
#include "quill/detail/misc/Macros.h"
#include "quill/detail/record/CommandRecord.h"
#include "quill/detail/record/LogRecord.h"
#include <gtest/gtest.h>
#include <string>

using namespace quill::detail;
using namespace quill;

TEST(Record, construct)
{
  HandlerCollection hc;
  constexpr StaticLogRecordInfo log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test fmt {}", quill::LogLevel::Debug};
  LoggerDetails logger_details{"default", hc.stdout_streamhandler()};
  {
    // test with char const the tuple get's promoted
    using record_t = LogRecord<int, double, char const*>;
    static_assert(std::is_same<record_t::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");

    // Try to contruct one using the same args
    record_t msg{&log_line_info, &logger_details, 1337, 13.5, "test"};

    // Check that the constructed msg has a promoted underlying tuple
    static_assert(std::is_same<decltype(msg)::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");
  }

  {
    // test with char*
    char test_char[] = "test";
    using record_t = LogRecord<int, double, char*>;
    static_assert(std::is_same<record_t::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");

    // Try to contruct one using the same args
    record_t msg(&log_line_info, &logger_details, 1337, 13.5, test_char);

    // Check that the constructed msg has a promoted underlying tuple
    static_assert(std::is_same<decltype(msg)::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");
  }
}