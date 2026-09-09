#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <thread>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#if defined(QUILL_TEST_MODULE)
import quill;
  #define QUILL_USE_MODULE
bool log_from_another_module_translation_unit();
#else
  #include "quill/Backend.h"
  #include "quill/Frontend.h"
  #include "quill/LogFunctions.h"
  #include "quill/sinks/ConsoleSink.h"
  #include "quill/sinks/FileSink.h"
  #include "quill/sinks/JsonSink.h"
  #include "quill/sinks/RotatingFileSink.h"
  #include "quill/std/Array.h"
  #include "quill/std/Chrono.h"
  #include "quill/std/Map.h"
  #include "quill/std/Optional.h"
  #include "quill/std/Variant.h"
  #include "quill/std/Vector.h"
#endif

#include "quill/LogMacros.h"

int main()
{
#if defined(QUILL_TEST_MODULE)
  std::string const prefix{"bazel_module_logging_test"};
#else
  std::string const prefix{"bazel_header_logging_test"};
#endif

  std::string const filename{prefix + ".log"};
  std::string const rotating_filename{prefix + "_rotating.log"};
  std::string const json_filename{prefix + ".json"};

  quill::Backend::start();

  quill::FileSinkConfig sink_config;
  sink_config.set_open_mode('w');
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(filename, sink_config);

  quill::RotatingFileSinkConfig rotating_config;
  rotating_config.set_open_mode('w');
  rotating_config.set_rotation_max_file_size(1024 * 1024);
  auto rotating_sink =
    quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(rotating_filename, rotating_config);

  auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>(prefix + "_console");
  console_sink->set_log_level_filter(quill::LogLevel::Error);

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    prefix + "_logger",
    std::vector<std::shared_ptr<quill::Sink>>{std::move(file_sink), std::move(rotating_sink),
                                              std::move(console_sink)},
    quill::PatternFormatterOptions{"%(message)"});
  logger->set_log_level(quill::LogLevel::Info);

  LOG_INFO(logger, "Bazel logging test: {}", 42);
  LOG_INFO(logger, "Basic values: {} {:.2f} {} {}", -7, 3.5, true, "text");
  LOG_DEBUG(logger, "Filtered debug message");
  quill::info(logger, "Macro-free logging: {}", 44);

  std::vector<int> const values{1, 2, 3};
  std::array<int, 2> const array{{4, 5}};
  std::map<std::string, int> const mapping{{"key", 7}};
  std::optional<int> const optional{9};
  std::variant<int, std::string> const variant{11};

  LOG_INFO(logger, "Vector: {}", values);
  LOG_INFO(logger, "Array: {}", array);
  LOG_INFO(logger, "Map: {}", mapping);
  LOG_INFO(logger, "Optional: {}", optional);
  LOG_INFO(logger, "Variant: {}", variant);
  LOG_INFO(logger, "Duration: {}", std::chrono::milliseconds{12});

  logger->init_backtrace(2, quill::LogLevel::Error);
  LOG_BACKTRACE(logger, "Buffered context: {}", 1);
  LOG_ERROR(logger, "Error flushes backtrace");

  std::thread producer{[logger]() { LOG_WARNING(logger, "Logging from another thread: {}", 45); }};
  producer.join();

  auto json_sink = quill::Frontend::create_or_get_sink<quill::JsonFileSink>(json_filename, sink_config);
  quill::Logger* json_logger = quill::Frontend::create_or_get_logger(
    prefix + "_json_logger", std::move(json_sink), quill::PatternFormatterOptions{""});
  LOG_INFO(json_logger, "Structured {answer}", 42);

  bool passed{true};
#if defined(QUILL_TEST_MODULE)
  passed = log_from_another_module_translation_unit();
#endif

  logger->flush_log();
  json_logger->flush_log();
  quill::Frontend::remove_logger_blocking(logger);
  quill::Frontend::remove_logger_blocking(json_logger);
  quill::Backend::stop();

  for (std::string const& log_filename : {filename, rotating_filename})
  {
    std::ifstream log_file{log_filename};
    std::string const output{std::istreambuf_iterator<char>{log_file}, std::istreambuf_iterator<char>{}};

    for (char const* expected :
         {"Bazel logging test: 42", "Basic values: -7 3.50 true text", "Macro-free logging: 44",
          "Vector: [1, 2, 3]", "Array: [4, 5]", "Map: {\"key\": 7}", "Optional: optional(9)",
          "Variant: variant(11)", "Duration: 12ms", "Buffered context: 1",
          "Error flushes backtrace", "Logging from another thread: 45"})
    {
      if (output.find(expected) == std::string::npos)
      {
        std::fprintf(stderr, "%s is missing: %s\n", log_filename.c_str(), expected);
        passed = false;
      }
    }

    passed = passed && output.find("Filtered debug message") == std::string::npos;
#if defined(QUILL_TEST_MODULE)
    passed = passed && output.find("Module logging from another translation unit: 43") != std::string::npos;
#endif

    log_file.close();
    std::remove(log_filename.c_str());
  }

  std::ifstream json_file{json_filename};
  std::string const json{std::istreambuf_iterator<char>{json_file}, std::istreambuf_iterator<char>{}};
  if (json.find("\"answer\":\"42\"") == std::string::npos)
  {
    std::fprintf(stderr, "Missing structured JSON field: %s\n", json.c_str());
    passed = false;
  }
  json_file.close();
  std::remove(json_filename.c_str());

  return passed ? 0 : 1;
}
