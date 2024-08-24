#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/bundled/fmt/ostream.h"
#include "quill/sinks/JsonFileSink.h"

#include <cstdio>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

class UserDefinedType
{
public:
  UserDefinedType() = default;
  UserDefinedType(size_t i, std::string const& s) : _i(i), _s(s) {}

  virtual ~UserDefinedType() = default;

  friend std::ostream& operator<<(std::ostream& os, UserDefinedType const& obj)
  {
    if (obj._i && obj._s)
    {
      os << "i: " << *obj._i << ", s: " << *obj._s;
    }

    return os;
  }

private:
  std::optional<size_t> _i;
  std::optional<std::string> _s;
};

template <>
struct fmtquill::formatter<UserDefinedType> : fmtquill::ostream_formatter
{
};

/***/
TEST_CASE("json_file_logging")
{
  static constexpr size_t number_of_messages = 500u;
  static constexpr size_t number_of_threads = 6;
  static constexpr char const* json_filename = "json_file_logging.json";
  static constexpr char const* filename = "json_file_logging_file.log";
  static std::string const logger_name_prefix = "logger_";

  // Start the logging backend thread
  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};
  Backend::start(bo);

  std::vector<std::thread> threads;

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    threads.emplace_back(
      [i]() mutable
      {
        // log to json
        auto json_file_sink = Frontend::create_or_get_sink<JsonFileSink>(
          json_filename,
          []()
          {
            JsonFileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
          }(),
          FileEventNotifier{});

        auto file_sink = Frontend::create_or_get_sink<FileSink>(
          filename,
          []()
          {
            FileSinkConfig cfg;
            cfg.set_open_mode('w');
            return cfg;
          }(),
          FileEventNotifier{});

        Logger* logger = Frontend::create_or_get_logger(
          logger_name_prefix + std::to_string(i),
          std::initializer_list<std::shared_ptr<Sink>>{std::move(json_file_sink), std::move(file_sink)},
          quill::PatternFormatterOptions{
            "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) %(logger:<12) "
            "%(message) [%(named_args)]"});

        if (i == 0)
        {
          // log a message without any args, only from the first thread
          LOG_INFO(logger, "Hello from thread");

          // Log a message with non-printable chars
          const char* npcs = "Example\u0003String\u0004";

          LOG_INFO(logger, "contains non-printable {npcs}", npcs);
        }

        for (size_t j = 0; j < number_of_messages; ++j)
        {
          LOG_INFO(logger,
                   "Hello from thread {thread_index} this is message {message_num} [{custom}, "
                   "{double:.2f}]",
                   i, j, fmtquill::format("{}", UserDefinedType{j, std::to_string(j)}), 3.17312);
        }

        if (i == 0)
        {
          // log an invalid format for testing, only from the first thread
          LOG_INFO(logger, "invalid format [{%f}]", 321.1);
        }
      });
  }

  for (auto& elem : threads)
  {
    elem.join();
  }

  // flush all log and remove all loggers
  for (Logger* logger : Frontend::get_all_loggers())
  {
    logger->flush_log();
    Frontend::remove_logger(logger);
  }

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(json_filename);
  std::vector<std::string> const file_contents_s = quill::testing::file_contents(filename);

  REQUIRE_EQ(file_contents.size(), number_of_messages * number_of_threads + 3);
  REQUIRE_EQ(file_contents_s.size(), number_of_messages * number_of_threads + 3);

  for (size_t i = 0; i < number_of_threads; ++i)
  {
    // for each thread
    for (size_t j = 0; j < number_of_messages; ++j)
    {
      // check json log
      std::string expected_json_string = std::string{"\"logger\":\""} + logger_name_prefix +
        std::to_string(i) +
        std::string{
          "\",\"log_level\":\"INFO\",\"message\":\"Hello from thread {thread_index} this is "
          "message {message_num} [{custom}, {double:.2f}]\","} +
        std::string{"\"thread_index\":\""} + std::to_string(i) +
        std::string{"\",\"message_num\":\""} + std::to_string(j) +
        std::string{"\",\"custom\":\"i: "} + std::to_string(j) + ", s: " + std::to_string(j) +
        std::string{"\",\"double\":\"3.17\""};

      REQUIRE(quill::testing::file_contains(file_contents, expected_json_string));

      // check standard log
      // for each thread [i: 0, s: 0]
      std::string expected_string = logger_name_prefix + std::to_string(i) +
        "     Hello from thread " + std::to_string(i) + " this is message " + std::to_string(j) +
        +" [i: " + std::to_string(j) + ", s: " + std::to_string(j) +
        ", 3.17] [thread_index: " + std::to_string(i) + ", message_num: " + std::to_string(j) + ", ";

      REQUIRE(quill::testing::file_contains(file_contents_s, expected_string));
    }

    std::string expected_no_args_json = "\"log_level\":\"INFO\",\"message\":\"Hello from thread\"";
    std::string expected_no_args_fmt = "Hello from thread";
    REQUIRE(quill::testing::file_contains(file_contents, expected_no_args_json));
    REQUIRE(quill::testing::file_contains(file_contents_s, expected_no_args_fmt));

    std::string expected_non_printable_json = "\"npcs\":\"Example\\x03String\\x04\"";
    std::string expected_non_printable_fmt = "contains non-printable Example\\x03String\\x04";
    REQUIRE(quill::testing::file_contains(file_contents, expected_non_printable_json));
    REQUIRE(quill::testing::file_contains(file_contents_s, expected_non_printable_fmt));

    std::string expected_invalid_fmt_json =
      "\"log_level\":\"INFO\",\"message\":\"invalid format [{%f}]\"";
    std::string expected_invalid_fmt = "invalid format [{%f}]\", location: \"";
    REQUIRE(quill::testing::file_contains(file_contents, expected_invalid_fmt_json));
    REQUIRE(quill::testing::file_contains(file_contents_s, expected_invalid_fmt));
  }

  testing::remove_file(json_filename);
  testing::remove_file(filename);
}
