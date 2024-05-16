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
TEST_CASE("string_random_small_logging")
{
  static constexpr char const* filename = "string_random_small_logging.log";
  static std::string const logger_name = "logger";
  static constexpr size_t number_of_strings = 500;
  static constexpr int min_string_len = 1;
  static constexpr int max_string_len = 50;

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

  std::vector<std::string> random_strings_vec =
    quill::testing::gen_random_strings(number_of_strings, min_string_len, max_string_len);

  // First push in sequence
  for (auto const& elem : random_strings_vec)
  {
    LOG_INFO(logger, "{}", elem);
    LOG_INFO(logger, "{}", elem.c_str());
    LOG_INFO(logger, "{}", std::string_view{elem});
    logger->flush_log();
  }

  for (auto const& elem : random_strings_vec)
  {
    LOG_INFO(logger, "{}", elem);
    logger->flush_log();
  }

  for (auto const& elem : random_strings_vec)
  {
    LOG_INFO(logger, "{}", elem.c_str());
    logger->flush_log();
  }

  for (auto const& elem : random_strings_vec)
  {
    LOG_INFO(logger, "{}", std::string_view{elem});
    logger->flush_log();
  }

  // Then also try to push all
  for (auto const& elem : random_strings_vec)
  {
    LOG_INFO(logger, "{}", elem);
    LOG_INFO(logger, "{}", elem.c_str());
    LOG_INFO(logger, "{}", std::string_view{elem});
  }

  // clear the vector for the strings to go out of scope for additional testing
  size_t const total_log_messages = random_strings_vec.size() * 9;
  random_strings_vec.clear();

  // Let all log get flushed to the file
  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check we logged everything
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), total_log_messages);

  testing::remove_file(filename);
}