#include <gtest/gtest.h>

#include "quill/detail/Message.h"
#include "quill/detail/MessageHelpers.h"
#include "quill/sinks/StdoutSink.h"
#include <string>

using namespace quill::detail;
using namespace quill;

TEST(Message, construct)
{
  constexpr LogLineInfo log_line_info{__LINE__, __FILE__, __FUNCTION__, "Test fmt {}", quill::LogLevel::Debug};
  LoggerDetails logger_details{"default", std::make_unique<StdoutSink>()};
  {
    // test with char const
    using MessageT = Message<int, double, char const*>;
    static_assert(std::is_same_v<MessageT::PromotedTupleT, std::tuple<int, double, std::string>>,
                  "tuple is not promoted");

    MessageT msg{&log_line_info, &logger_details, 1337, 13.5, "test"};
  }

  {
    // test with char*
    char test_char[] = "test";
    using MessageT = Message<int, double, char*>;
    static_assert(std::is_same_v<MessageT::PromotedTupleT, std::tuple<int, double, std::string>>,
                  "tuple is not promoted");

    MessageT msg{&log_line_info, &logger_details, 1337, 13.5, test_char};
  }
}

TEST(CommandMessage, construct)
{
  CommandMessage msg{[]() {}};
}