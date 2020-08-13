#include "doctest/doctest.h"

#include "quill/detail/HandlerCollection.h"
#include "quill/detail/events/FlushEvent.h"
#include "quill/detail/events/LogRecordEvent.h"
#include "quill/detail/misc/Macros.h"
#include <string>

TEST_SUITE_BEGIN("Record");

using namespace quill::detail;
using namespace quill;

struct mock_log_record_info
{
  constexpr quill::detail::LogRecordMetadata operator()() { return LogRecordMetadata{}; }
};

constexpr bool is_backtrace_log_record{false};

TEST_CASE("construct")
{
  HandlerCollection hc;

  LoggerDetails logger_details{"default", hc.stdout_console_handler()};
  {
    // test with char const the tuple get's promoted
    using record_t = LogRecordEvent<is_backtrace_log_record, mock_log_record_info, int, double, char const*>;
    static_assert(std::is_same<record_t::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");

    // Try to contruct one using the same args
    record_t msg{&logger_details, 1337, 13.5, "test"};

    // Check that the constructed msg has a promoted underlying tuple
    static_assert(std::is_same<decltype(msg)::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");
  }

  {
    // test with char*
    char test_char[] = "test";
    using record_t = LogRecordEvent<is_backtrace_log_record, mock_log_record_info, int, double, char*>;
    static_assert(std::is_same<record_t::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");

    // Try to contruct one using the same args
    record_t msg(&logger_details, 1337, 13.5, test_char);

    // Check that the constructed msg has a promoted underlying tuple
    static_assert(std::is_same<decltype(msg)::PromotedTupleT, std::tuple<int, double, std::string>>::value,
                  "tuple is not promoted");
  }
}

TEST_SUITE_END();