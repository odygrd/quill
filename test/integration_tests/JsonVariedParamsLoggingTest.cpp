#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/JsonSink.h"

#include <cstdio>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace quill;

class CustomJsonFileSink : public JsonFileSink
{
public:
  using JsonFileSink::JsonFileSink;

  void write_log(LogRecord const& record) override
  {
    _json_message.clear();

    _json_message.append(fmtquill::format(R"({{"logger":"{}","log_level":"{}","message":"{}")",
                                          record.logger_name, record.log_level_description,
                                          record.log_metadata->message_format()));

    if (record.named_args)
    {
      for (auto const& [key, value] : *record.named_args)
      {
        _json_message.append(std::string_view{",\""});
        _json_message.append(key);
        _json_message.append(std::string_view{"\":\""});
        _json_message.append(value);
        _json_message.append(std::string_view{"\""});
      }
    }

    _json_message.append(std::string_view{"}\n"});

    LogRecord json_record = record;
    json_record.log_message = std::string_view{};
    json_record.log_statement = std::string_view{_json_message.data(), _json_message.size()};
    FileSink::write_log(json_record);
  }

private:
  fmtquill::memory_buffer _json_message;
};

/**
 * Tests that named args are correctly cleared by logging several messages with and without parameters
 **/
TEST_CASE("json_varied_params_logging")
{
  static constexpr size_t number_of_messages = 127u;
  static constexpr char const* filename = "json_varied_params_logging.json";

  // Start the logging backend thread
  BackendOptions bo;
  bo.error_notifier = [](std::string const&) {};

  // configure a small transit event capacity to make the transit_events reused
  bo.transit_event_buffer_initial_capacity = 16;
  Backend::start(bo);

  std::vector<std::thread> threads;

  // log to json
  auto json_file_sink = Frontend::create_or_get_sink<CustomJsonFileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(
    "root", std::move(json_file_sink),
    quill::PatternFormatterOptions{"LOG_%(log_level:<9) %(logger:<12) %(message) [%(named_args)]"});

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    if (i % 3 == 0)
    {
      LOG_INFO(logger, "No parameters");
    }
    else if (i % 3 == 1)
    {
      LOG_WARNING(logger, "One parameter {param_1}", i);
    }
    else
    {
      LOG_ERROR(logger, "Two parameters {param_1} {param_2}", i, i);
    }

    // flush the log each time so that the transit events are reused
    logger->flush_log();
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    if (i % 3 == 0)
    {
      std::string const expected_json_statement = R"({"logger":"root","log_level":"INFO","message":"No parameters"})";

      REQUIRE_EQ(file_contents[i], expected_json_statement);
    }
    else if (i % 3 == 1)
    {
      std::string const expected_json_statement = fmtquill::format(
        R"({{"logger":"root","log_level":"WARNING","message":"One parameter {{param_1}}","param_1":"{}"}})", i);

      REQUIRE_EQ(file_contents[i], expected_json_statement);
    }
    else
    {
      std::string const expected_json_statement = fmtquill::format(
        R"({{"logger":"root","log_level":"ERROR","message":"Two parameters {{param_1}} {{param_2}}","param_1":"{}","param_2":"{}"}})",
        i, i);

      REQUIRE_EQ(file_contents[i], expected_json_statement);
    }
  }

  testing::remove_file(filename);
}
