#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

struct UserSink final : public quill::Sink
{
  UserSink() = default;

  /***/
  void write_log(quill::MacroMetadata const* /** log_metadata **/, uint64_t /** log_timestamp **/,
                 std::string_view /** thread_id **/, std::string_view /** thread_name **/,
                 std::string const& /** process_id **/, std::string_view /** logger_name **/,
                 quill::LogLevel /** log_level **/, std::string_view /** log_level_description **/,
                 std::string_view /** log_level_short_code **/,
                 std::vector<std::pair<std::string, std::string>> const* /** named_args **/,
                 std::string_view /** log_message **/, std::string_view /** log_statement **/) override
  {
    log_statement_cnt.fetch_add(1);
  }

  /***/
  void flush_sink() noexcept override { flush_sink_cnt.fetch_add(1); }

  /***/
  void run_periodic_tasks() noexcept override { periodic_tasks_cnt.fetch_add(1); }

  std::atomic<uint64_t> log_statement_cnt{0};
  std::atomic<uint64_t> flush_sink_cnt{0};
  std::atomic<uint64_t> periodic_tasks_cnt{0};
};

/***/
TEST_CASE("user_sink")
{
  static constexpr size_t number_of_messages = 10;
  static std::string const logger_name_a = "logger_a";
  static std::string const logger_name_b = "logger_b";
  static std::string const logger_name_c = "logger_c";

  // Start the logging backend thread
  Backend::start();

  // Set writing logging to a file
  auto user_sink_a = Frontend::create_or_get_sink<UserSink>("sink_a");
  Logger* logger_a = Frontend::create_or_get_logger(logger_name_a, user_sink_a);

  auto user_sink_b = Frontend::create_or_get_sink<UserSink>("sink_b");
  Logger* logger_b = Frontend::create_or_get_logger(logger_name_b, user_sink_b);

  // logger c is using user_sink_b
  auto user_sink_b_ref = Frontend::get_sink("sink_b");
  Logger* logger_c = Frontend::create_or_get_logger(logger_name_c, user_sink_b_ref);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    // log an array so the log message is pushed to the queue
    LOG_INFO(logger_a, "Lorem ipsum dolor sit amet, consectetur adipiscing elit {}", i);
  }

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_ERROR(logger_b, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {}", i);
  }

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    LOG_ERROR(logger_c, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {}", i);
  }

  // Let the backend worker run a few times so that flush_sink_cnt and periodic_tasks_cnt are called
  constexpr uint32_t max_retries = 20;
  constexpr uint32_t min_flushes = 20;
  uint32_t retry_count = 0;

  // user_sink_b is created second after user_sink_a
  // if user_sink_b has at least min_flushes, then user_sink_a should have them too
  while (reinterpret_cast<UserSink*>(user_sink_b.get())->flush_sink_cnt.load() < min_flushes)
  {
    if (retry_count >= max_retries)
    {
      FAIL("Exceeded maximum retry count");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    ++retry_count;
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // user_sink_b will have twice the messages since logger_c is also logging there
  REQUIRE_EQ(reinterpret_cast<UserSink*>(user_sink_a.get())->log_statement_cnt.load(), number_of_messages);
  REQUIRE_EQ(reinterpret_cast<UserSink*>(user_sink_b.get())->log_statement_cnt.load(), number_of_messages * 2);

  uint64_t const count_a_flush = reinterpret_cast<UserSink*>(user_sink_a.get())->flush_sink_cnt.load();
  uint64_t const count_b_flush = reinterpret_cast<UserSink*>(user_sink_b.get())->flush_sink_cnt.load();
  uint64_t const count_a_periodic =
    reinterpret_cast<UserSink*>(user_sink_a.get())->periodic_tasks_cnt.load();
  uint64_t const count_b_periodic =
    reinterpret_cast<UserSink*>(user_sink_b.get())->periodic_tasks_cnt.load();

  REQUIRE_GE(count_a_flush, 10);
  REQUIRE_GE(count_b_flush, 10);
  REQUIRE_GE(count_a_periodic, 10);
  REQUIRE_GE(count_b_flush, 10);

  // since logger_a is created first and then logger_b it is expected that the counts will
  // be different in each sink, but they should be close to each other
  int64_t const threshold = 20;

  // Check if counts are close to each other
  REQUIRE_LE(std::abs(static_cast<int64_t>(count_a_flush - count_b_flush)), threshold);
  REQUIRE_LE(std::abs(static_cast<int64_t>(count_a_periodic - count_b_periodic)), threshold);
}