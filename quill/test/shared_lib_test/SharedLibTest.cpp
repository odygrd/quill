#include "doctest/doctest.h"

#include "foo/Foo.h"
#include "misc/TestUtilities.h"

#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"

/**
 * Tests the process of building a Quill wrapper shared library and initializing it within another
 * shared library, 'foo', which we are linking against. This test is particularly useful on
 * platforms like Windows, where shared libraries have different default visibilities, potentially
 * causing issues with internal singleton classes used by the library.
 */

__declspec(dllimport) extern quill::Logger* global_logger_a;

/***/
TEST_CASE("quill_shared_lib")
{
  init();
  log();

  quill::Logger* logger = quill::Frontend::get_logger("root");
  LOG_INFO(logger, "log with logger from {}", "test");

  // Log via the global logger
  LOG_INFO(global_logger_a, "log with global logger from {}", "test");

  std::thread t1{[logger]() { LOG_INFO(logger, "log with logger from {}", "test thread"); }};
  t1.join();

  logger->flush_log();
  quill::Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  stop();

  // Read file and check
  std::vector<std::string> const file_contents =
    quill::testing::file_contents("quill_shared_lib_test.log");
  REQUIRE_EQ(file_contents.size(), 6);

  std::string logger_name{"root"};

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with logger from foo"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with global logger from foo"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with logger from foo thread"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with logger from test"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with global logger from test"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "         log with logger from test thread"}));

  quill::testing::remove_file("quill_shared_lib_test.log");
}