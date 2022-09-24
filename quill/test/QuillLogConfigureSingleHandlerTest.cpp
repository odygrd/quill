#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Quill.h"
#include "quill/detail/misc/FileUtilities.h"
#include <cstdio>
#include <string>

TEST_SUITE_BEGIN("QuillLogConfigure");

/***/
TEST_CASE("log_configure_default_logger_single_handler")
{
  static constexpr char const* filename = "log_configure_default_logger_single_handler.log";

  // get a pointer to the default logger
  quill::Logger* logger = quill::get_logger();

  // Config using the custom ts class and the stdout handler
  // this should not invalidate the default logger
  quill::Config cfg;
  quill::Handler* file_handler = quill::file_handler(filename, "w");
  cfg.default_handlers.emplace_back(file_handler);
  cfg.default_logger_name = "root_test";
  quill::configure(cfg);

  // Start the backend logging thread
  quill::start();

  // check that we did update the default logger
  auto all_loggers = quill::get_all_loggers();
  REQUIRE_EQ(all_loggers.size(), 1u);
  REQUIRE_NE(all_loggers.find(cfg.default_logger_name), all_loggers.end());

  // Log using the default logger pointer we obtained before the configure step
  // We expect the pointer to be valid after quill::configure(...)
  LOG_INFO(logger, "Hello from log_configure_default_logger test");
  LOG_INFO(logger, "The root logger pointer is valid");

  // Flush all log
  quill::flush();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  {
    std::string const expected_string =
      "LOG_INFO      root_test    - Hello from log_configure_default_logger test";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  {
    std::string const expected_string =
      "LOG_INFO      root_test    - The root logger pointer is valid";
    REQUIRE(quill::testing::file_contains(file_contents, expected_string));
  }

  quill::detail::remove_file(filename);
}

TEST_SUITE_END();