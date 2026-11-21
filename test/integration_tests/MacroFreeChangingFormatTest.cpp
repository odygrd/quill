#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogFunctions.h"
#include "quill/bundled/fmt/format.h"
#include "quill/sinks/FileSink.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("macro_free_changing_format")
{
  static constexpr char const* filename = "macro_free_changing_format.log";
  static std::string const logger_name = "macro_free_changing_format_logger";
  static constexpr size_t number_of_messages = 3000;

  // A transit event buffer of four events, so that the backend reuses each of them for messages
  // of every format string below
  BackendOptions backend_options;
  backend_options.transit_event_buffer_initial_capacity = 4;
  backend_options.transit_events_soft_limit = 4;
  backend_options.transit_events_hard_limit = 4;
  Backend::start(backend_options);

  auto file_sink = Frontend::create_or_get_sink<FileSink>(filename,
                                                          []()
                                                          {
                                                            FileSinkConfig cfg;
                                                            cfg.set_open_mode('w');
                                                            return cfg;
                                                          }());

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink), PatternFormatterOptions{"%(message) [%(named_args)]"});

  // Runs of messages with the same format string, written to the same buffer before each call
  std::vector<std::string> const formats{"positional {} and {}", "named {first} and {second:>3}",
                                         "spec [{:>4}] and {:#x}", "dynamic [{:>{}}]"};
  std::string format;
  format.reserve(64);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    size_t const format_index = (i / 5) % formats.size();
    format = formats[format_index];

    if (format_index == 3)
    {
      quill::info(logger, format.c_str(), i, 6);
    }
    else
    {
      quill::info(logger, format.c_str(), i, i + 1);
    }
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    size_t const format_index = (i / 5) % formats.size();
    std::string expected;

    if (format_index == 0)
    {
      expected = fmtquill::format("positional {} and {} []", i, i + 1);
    }
    else if (format_index == 1)
    {
      expected = fmtquill::format("named {} and {:>3} [first: {}, second: {:>3}]", i, i + 1, i, i + 1);
    }
    else if (format_index == 2)
    {
      expected = fmtquill::format("spec [{:>4}] and {:#x} []", i, i + 1);
    }
    else
    {
      expected = fmtquill::format("dynamic [{:>6}] []", i);
    }

    REQUIRE_EQ(file_contents[i], expected);
  }

  testing::remove_file(filename);
}
