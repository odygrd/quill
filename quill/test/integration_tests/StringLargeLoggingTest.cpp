#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

/***/
TEST_CASE("string_large_logging")
{
  static constexpr char const* filename = "string_large_logging.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_messages = 100;

  // Start the logging backend thread
  Backend::start();

  Frontend::preallocate();

  // Set writing logging to a file
  auto file_sink = Frontend::create_or_get_sink<FileSink>(
    filename,
    []()
    {
      FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    FileEventNotifier{});

  Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

  for (size_t i = 0; i < number_of_messages; ++i)
  {
    std::string v{
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
      "incididunt ut labore et dolore magna aliqua. Dui accumsan sit amet nulla facilisi "
      "morbi tempus. Diam ut venenatis tellus in metus vulputate eu scelerisque felis. Lorem "
      "mollis aliquam ut porttitor leo a. Posuere urna nec tincidunt praesent semper feugiat "
      "nibh sed. Auctor urna nunc id cursus metus aliquam eleifend mi. Et ultrices neque ornare "
      "aenean euismod elementum nisi quis. Phasellus vestibulum lorem sed risus ultricies "
      "tristique nulla. Porta nibh venenatis cras sed felis eget velit aliquet sagittis. "
      "Eget arcu dictum varius duis at consectetur lorem. Diam quam nulla porttitor massa id "
      "neque aliquam vestibulum morbi. Sed euismod nisi porta lorem mollis aliquam. Arcu "
      "felis bibendum ut tristique. Lorem ipsum dolor sit amet consectetur adipiscing elit "
      "pellentesque habitant. Mauris augue neque gravida in. Dictum fusce ut placerat orci "
      "nulla pellentesque dignissim "};

    v += std::to_string(i);

    LOG_INFO(logger, "Logging int: {}, int: {}, string: {}, string: {}", i, i * 10, v, v);
  }

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), number_of_messages);

  testing::remove_file(filename);
}