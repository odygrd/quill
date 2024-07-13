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
TEST_CASE("variable_logging")
{
  static constexpr char const* filename = "variable_logging.log";
  static std::string const logger_name = "logger";

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
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOGV_INFO(logger, "Log message without argument");

  int var_1 = 123;
  LOGV_TRACE_L3(logger, "Log message", var_1);

  int var_2 = 456;
  LOGV_TRACE_L2(logger, "Log message", var_1, var_2);

  int var_3 = 789;
  LOGV_TRACE_L1(logger, "Log message", var_1, var_2, var_3);

  int var_4 = 101;
  LOGV_DEBUG(logger, "Log message", var_1, var_2, var_3, var_4);

  int var_5 = 102;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5);

  int var_6 = 103;
  LOGV_WARNING(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6);

  int var_7 = 104;
  LOGV_ERROR(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7);

  int var_8 = 105;
  LOGV_CRITICAL(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8);

  int var_9 = 106;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9);

  int var_10 = 107;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10);

  int var_11 = 108;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11);

  int var_12 = 109;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12);

  int var_13 = 110;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13);

  int var_14 = 111;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14);

  int var_15 = 112;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15);

  int var_16 = 113;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16);

  int var_17 = 114;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17);

  int var_18 = 115;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18);

  int var_19 = 116;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19);

  int var_20 = 117;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20);

  int var_21 = 118;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10,
            var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20, var_21);

  int var_22 = 119;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10,
            var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20, var_21, var_22);

  int var_23 = 120;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23);

  int var_24 = 121;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24);

  int var_25 = 122;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24, var_25);

  int var_26 = 123;
  LOGV_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24, var_25, var_26);

  LOGJ_INFO(logger, "Log message without named argument");
  LOGJ_TRACE_L3(logger, "Log message", var_1);
  LOGJ_TRACE_L2(logger, "Log message", var_1, var_2);
  LOGJ_TRACE_L1(logger, "Log message", var_1, var_2, var_3);
  LOGJ_DEBUG(logger, "Log message", var_1, var_2, var_3, var_4);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5);
  LOGJ_WARNING(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6);
  LOGJ_ERROR(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7);
  LOGJ_CRITICAL(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10,
            var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20, var_21);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10,
            var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20, var_21, var_22);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24, var_25);
  LOGJ_INFO(logger, "Log message", var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9,
            var_10, var_11, var_12, var_13, var_14, var_15, var_16, var_17, var_18, var_19, var_20,
            var_21, var_22, var_23, var_24, var_25, var_26);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message without argument"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L3  " + logger_name + "       Log message [var_1: 123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L2  " + logger_name + "       Log message [var_1: 123, var_2: 456]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L1  " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_DEBUG     " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_WARNING   " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118, var_22: 119]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118, var_22: 119, var_23: 120]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118, var_22: 119, var_23: 120, var_24: 121]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118, var_22: 119, var_23: 120, var_24: 121, var_25: 122]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message [var_1: 123, var_2: 456, var_3: 789, var_4: 101, var_5: 102, var_6: 103, var_7: 104, var_8: 105, var_9: 106, var_10: 107, var_11: 108, var_12: 109, var_13: 110, var_14: 111, var_15: 112, var_16: 113, var_17: 114, var_18: 115, var_19: 116, var_20: 117, var_21: 118, var_22: 119, var_23: 120, var_24: 121, var_25: 122, var_26: 123]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message without named argument"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L3  " + logger_name + "       Log message 123"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L2  " + logger_name + "       Log message 123, 456"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_TRACE_L1  " + logger_name + "       Log message 123, 456, 789"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_DEBUG     " + logger_name + "       Log message 123, 456, 789, 101"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_WARNING   " + logger_name + "       Log message 123, 456, 789, 101, 102, 103"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_CRITICAL  " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108"}));

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_INFO      " + logger_name + "       Log message 123, 456, 789, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123"}));

  testing::remove_file(filename);
}