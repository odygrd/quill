#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("start_stop_backend_worker")
{
  for (size_t iter = 0; iter < 4; ++iter)
  {
    static constexpr size_t number_of_messages = 200;
    static constexpr char const* filename = "start_stop_backend_worker.log";
    static std::string const logger_name = "logger";

    // Start the logging backend thread
    Backend::start();
    REQUIRE_EQ(Backend::is_running(), true);

    // Set writing logging to a file
    auto file_sink = Frontend::create_or_get_sink<FileSink>(
      filename,
      []()
      {
        FileSinkConfig cfg;
        cfg.set_open_mode('w');

        // For this test only we use the default buffer size, it should not make any difference it is just for testing the default behaviour and code coverage
        cfg.set_write_buffer_size(0);

        return cfg;
      }(),
      FileEventNotifier{});

    Logger* logger = Frontend::create_or_get_logger(logger_name, std::move(file_sink));

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      LOG_INFO(logger, "This is message {}", i);
    }

    // we do not call flush log or remove_logger_blocking to also check that stop() will do those
    Frontend::remove_logger(logger);

    // Wait until the backend thread stops
    Backend::stop();
    REQUIRE_EQ(Backend::get_thread_id(), 0);
    REQUIRE_EQ(Backend::is_running(), false);
    REQUIRE_EQ(Frontend::get_number_of_loggers(), 0);

    // Read file and check
    std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
    REQUIRE_EQ(file_contents.size(), number_of_messages);

    for (size_t i = 0; i < number_of_messages; ++i)
    {
      std::string expected_string = logger_name + "       This is message " + std::to_string(i);
      REQUIRE(quill::testing::file_contains(file_contents, expected_string));
    }

    testing::remove_file(filename);
  }
}