#include <gtest/gtest.h>

#include <string>

#include "quill/detail/record/CommandRecord.h"
#include "quill/detail/record/LogRecord.h"
#include "quill/detail/record/LogRecordHelpers.h"
#include "quill/sinks/StdoutSink.h"

using namespace quill::detail;
using namespace quill;

TEST(Record, construct)
{
  constexpr LogLineInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, "Test fmt {}", quill::LogLevel::Debug};
  LoggerDetails logger_details{"default", std::make_unique<StdoutSink>()};
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
  CommandRecord msg{[]() {}};
}