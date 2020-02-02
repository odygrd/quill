#include <gtest/gtest.h>

#include <string>

#include "quill/detail/HandlerCollection.h"
#include "quill/detail/record/CommandRecord.h"
#include "quill/detail/record/LogRecord.h"
#include "quill/detail/record/LogRecordUtilities.h"

using namespace quill::detail;
using namespace quill;

TEST(Record, construct)
{
  HandlerCollection hc;
  constexpr StaticLogRecordInfo log_line_info{QUILL_STRINGIFY(__LINE__), __FILE__, __FUNCTION__,
                                              "Test fmt {}", quill::LogLevel::Debug};
  LoggerDetails logger_details{"default", hc.stdout_streamhandler()};
  {
    // test with char const
    using record_t = LogRecord<int, double, char const*>;
    static_assert(std::is_same_v<record_t::PromotedTupleT, std::tuple<int, double, std::string>>,
                  "tuple is not promoted");

    record_t msg{&log_line_info, &logger_details, 1337, 13.5, "test"};
  }

  {
    // test with char*
    char test_char[] = "test";
    using record_t = LogRecord<int, double, char*>;
    static_assert(std::is_same_v<record_t::PromotedTupleT, std::tuple<int, double, std::string>>,
                  "tuple is not promoted");

    record_t msg{&log_line_info, &logger_details, 1337, 13.5, test_char};
  }
}

TEST(CommandRecord, construct)
{
  // TODO:: test
  CommandRecord msg{[]() {}};
}