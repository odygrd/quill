#include "doctest/doctest.h"

#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/SimpleSetup.h"
#include "quill/sinks/StreamSink.h"

#include <memory>
#include <string>

using namespace quill;

TEST_CASE("simple_setup_console_stream")
{
  Logger* stdout_logger = simple_logger("stdout");
  Logger* stderr_logger = simple_logger("stderr");

  auto stdout_sink = std::static_pointer_cast<StreamSink>(Frontend::get_sink("stdout"));
  auto stderr_sink = std::static_pointer_cast<StreamSink>(Frontend::get_sink("stderr"));

  REQUIRE_EQ(stdout_sink->get_filename().string(), std::string{"stdout"});
  REQUIRE_EQ(stderr_sink->get_filename().string(), std::string{"stderr"});

  Frontend::remove_logger_blocking(stdout_logger);
  Frontend::remove_logger_blocking(stderr_logger);
  Backend::stop();
}
