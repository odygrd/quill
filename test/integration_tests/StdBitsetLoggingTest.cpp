#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include "quill/std/Bitset.h"

#include <bitset>
#include <cstdio>
#include <string>
#include <vector>

using namespace quill;

/***/
TEST_CASE("std_bitset_logging")
{
  static constexpr char const* filename = "std_bitset_logging.log";
  static std::string const logger_name = "std_bitset_logger";

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

  std::bitset<8> const byte_bits{"10100101"};
  std::bitset<13> const packed_bits{"1010110010110"};
  std::bitset<1> const single_bit{"1"};
  std::bitset<16> const all_zero_bits{};
  std::bitset<16> const all_one_bits{"1111111111111111"};
  std::bitset<64> const wide_bits{
    "1000000000000000000000000000000000000000000000000000000000000001"};

  LOG_INFO(logger, "byte_bits {}", byte_bits);
  LOG_INFO(logger, "packed_bits {}", packed_bits);
  LOG_INFO(logger, "single_bit {}", single_bit);
  LOG_INFO(logger, "all_zero_bits {}", all_zero_bits);
  LOG_INFO(logger, "all_one_bits {}", all_one_bits);
  LOG_INFO(logger, "wide_bits {}", wide_bits);
  LOG_INFO(logger, "temp_bits {}", std::bitset<10>{"1100110011"});

  logger->flush_log();
  Frontend::remove_logger(logger);
  Backend::stop();

  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " byte_bits " + fmtquill::format("{}", byte_bits)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " packed_bits " + fmtquill::format("{}", packed_bits)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " single_bit " + fmtquill::format("{}", single_bit)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " all_zero_bits " + fmtquill::format("{}", all_zero_bits)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " all_one_bits " + fmtquill::format("{}", all_one_bits)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " wide_bits " + fmtquill::format("{}", wide_bits)));

  REQUIRE(quill::testing::file_contains(
    file_contents, logger_name + " temp_bits " + fmtquill::format("{}", std::bitset<10>{"1100110011"})));

  testing::remove_file(filename);
}
