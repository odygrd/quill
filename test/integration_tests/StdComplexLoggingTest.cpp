#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Complex.h"

#include <complex>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_complex_logging")
{
  static constexpr char const* filename = "std_complex_logging.log";
  static std::string const logger_name = "std_complex_logger";

  Backend::start();

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

  std::complex<double> const full_complex{1.25, -2.5};
  std::complex<double> const imaginary_only{0.0, 4.75};
  std::complex<double> const zero_complex{0.0, 0.0};
  std::complex<double> const real_only{3.5, 0.0};

  LOG_INFO(logger, "full_complex {}", full_complex);
  LOG_INFO(logger, "imaginary_only {}", imaginary_only);
  LOG_INFO(logger, "zero_complex {}", zero_complex);
  LOG_INFO(logger, "real_only {}", real_only);
  LOG_INFO(logger, "temp_complex {}", std::complex<float>{-3.0f, 2.0f});

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " full_complex (1.25-2.5i)"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " imaginary_only 4.75i"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " zero_complex 0i"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " real_only (3.5+0i)"));

  REQUIRE(quill::testing::file_contains(file_contents, logger_name + " temp_complex (-3+2i)"));

  testing::remove_file(filename);
}
