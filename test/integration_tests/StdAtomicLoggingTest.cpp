#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Atomic.h"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_atomic_logging")
{
  static constexpr char const* filename = "std_atomic_logging.log";
  static std::string const logger_name = "std_atomic_logger";

  Frontend::preallocate();

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

  std::atomic<bool> const boolean{true};
  std::atomic<double> const dbl{1.4};
  std::atomic<float> const flt{2.5f};
  std::atomic<int> counter{-42};

  LOG_INFO(logger, "bool_atomic {}", boolean);
  LOG_INFO(logger, "double_atomic {}", dbl);
  LOG_INFO(logger, "float_atomic {}", flt);
  LOG_INFO(logger, "temp_atomic {}", std::atomic<uint16_t>{65534});

  LOG_INFO(logger, "counter_before {}", counter);
  counter.store(17, std::memory_order_relaxed);
  LOG_INFO(logger, "counter_after {}", counter);
  counter.store(99, std::memory_order_relaxed);

  // Process the records only after the counter changes to verify the captured values.
  Backend::start();
  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " bool_atomic true"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " double_atomic 1.4"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " float_atomic 2.5"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " temp_atomic 65534"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " counter_before -42"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " counter_after 17"));

  testing::remove_file(filename);
}
