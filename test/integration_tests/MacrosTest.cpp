#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/***/
TEST_CASE("macros")
{
  static constexpr char const* filename = "macros.log";
  static std::string const logger_name = "logger";

  quill::Backend::start();

  // Set writing logging to a file
  auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
    filename,
    []()
    {
      quill::FileSinkConfig cfg;
      cfg.set_open_mode('w');
      return cfg;
    }(),
    quill::FileEventNotifier{});

  quill::Logger* logger = quill::Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    quill::PatternFormatterOptions{"%(time) [%(thread_id)] %(short_source_location:<28) "
                                   "%(log_level_short_code) LOG_%(log_level:<9) "
                                   "%(logger:<12) %(message) [ %(tags)] [%(named_args)]"});
  logger->init_backtrace(1, quill::LogLevel::Error);
  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3(logger, "L3: {}", 1);
  LOG_TRACE_L2(logger, "L2: {}", 2);
  LOG_TRACE_L1(logger, "L1: {}", 3);
  LOG_DEBUG(logger, "DBG: {}", 4);
  LOG_INFO(logger, "INF: {}", 5);
  LOG_NOTICE(logger, "NTC: {}", "N");
  LOG_WARNING(logger, "WRN: {}", "W");
  LOG_ERROR(logger, "ERR: {}", 6.78);
  LOG_CRITICAL(logger, "CRT: {}", -1);
  LOG_BACKTRACE(logger, "BT: {}", 255);
  LOG_DYNAMIC(logger, quill::LogLevel::Critical, "DYN: {}", 42);

  LOG_TRACE_L3_LIMIT(std::chrono::nanoseconds{0}, logger, "B L3: {}", 10);
  LOG_TRACE_L2_LIMIT(std::chrono::nanoseconds{0}, logger, "B L2: {}", 20);
  LOG_TRACE_L1_LIMIT(std::chrono::nanoseconds{0}, logger, "B L1: {}", 30);
  LOG_DEBUG_LIMIT(std::chrono::nanoseconds{0}, logger, "B DBG: {}", 40);
  LOG_INFO_LIMIT(std::chrono::nanoseconds{0}, logger, "B INF: {}", 50);
  LOG_NOTICE_LIMIT(std::chrono::nanoseconds{0}, logger, "B NTC: {}", "Notice");
  LOG_WARNING_LIMIT(std::chrono::nanoseconds{0}, logger, "B WRN: {}", "Warning");
  LOG_ERROR_LIMIT(std::chrono::nanoseconds{0}, logger, "B ERR: {}", 60.78);
  LOG_CRITICAL_LIMIT(std::chrono::nanoseconds{0}, logger, "B CRT: {}", -10);

  LOG_TRACE_L3_LIMIT_EVERY_N(1, logger, "BA L3: {}", 10);
  LOG_TRACE_L2_LIMIT_EVERY_N(1, logger, "BA L2: {}", 20);
  LOG_TRACE_L1_LIMIT_EVERY_N(1, logger, "BA L1: {}", 30);
  LOG_DEBUG_LIMIT_EVERY_N(1, logger, "BA DBG: {}", 40);
  LOG_INFO_LIMIT_EVERY_N(1, logger, "BA INF: {}", 50);
  LOG_NOTICE_LIMIT_EVERY_N(1, logger, "BA NTC: {}", "Notice");
  LOG_WARNING_LIMIT_EVERY_N(1, logger, "BA WRN: {}", "Warning");
  LOG_ERROR_LIMIT_EVERY_N(1, logger, "BA ERR: {}", 60.78);
  LOG_CRITICAL_LIMIT_EVERY_N(1, logger, "BA CRT: {}", -10);

  LOG_TRACE_L3_TAGS(logger, TAGS("tag"), "C L3: {}", 100);
  LOG_TRACE_L2_TAGS(logger, TAGS("tag"), "C L2: {}", 200);
  LOG_TRACE_L1_TAGS(logger, TAGS("tag"), "C L1: {}", 300);
  LOG_DEBUG_TAGS(logger, TAGS("tag"), "C DBG: {}", 400);
  LOG_INFO_TAGS(logger, TAGS("tag"), "C INF: {}", 500);
  LOG_NOTICE_TAGS(logger, TAGS("tag"), "C NTC: {}", "NoticeTag");
  LOG_WARNING_TAGS(logger, TAGS("tag"), "C WRN: {}", "WarningTag");
  LOG_ERROR_TAGS(logger, TAGS("tag"), "C ERR: {}", 600.78);
  LOG_CRITICAL_TAGS(logger, TAGS("tag"), "C CRT: {}", -100);
  LOG_BACKTRACE_TAGS(logger, TAGS("tag"), "C BT: {}", 255);
  LOG_DYNAMIC_TAGS(logger, quill::LogLevel::Critical, TAGS("tag"), "C DYN: {}", 420);

  LOG_RUNTIME_METADATA(logger, quill::LogLevel::Info, "MacrosTest.cpp", 1234, "function()",
                       "CA INF_{}", 11);
  LOG_RUNTIME_METADATA_SHALLOW(logger, quill::LogLevel::Info, "MacrosTest.cpp", 1234, "function()",
                               "", "CA INF_{}", 0);
  LOG_RUNTIME_METADATA_HYBRID(logger, quill::LogLevel::Debug, "MacrosTest.cpp", 1234, "function()",
                              "", "CA DBG_0");
  LOG_RUNTIME_METADATA_DEEP(logger, quill::LogLevel::Debug, "MacrosTest.cpp", 123, "function()",
                            "#TEST", "CA DBG_21");

  int var{1337};
  LOGV_TRACE_L3(logger, "D L3", var);
  LOGV_TRACE_L2(logger, "D L2", var);
  LOGV_TRACE_L1(logger, "D L1", var);
  LOGV_DEBUG(logger, "D DBG", var);
  LOGV_INFO(logger, "D INF", var);
  LOGV_NOTICE(logger, "D NTC", var);
  LOGV_WARNING(logger, "D WRN", var);
  LOGV_ERROR(logger, "D ERR", var);
  LOGV_CRITICAL(logger, "D CRT", var);
  LOGV_BACKTRACE(logger, "D BT", var);
  LOGV_DYNAMIC(logger, quill::LogLevel::Critical, "D DYN", var);

  LOGV_TRACE_L3_LIMIT(std::chrono::nanoseconds{0}, logger, "E L3", var);
  LOGV_TRACE_L2_LIMIT(std::chrono::nanoseconds{0}, logger, "E L2", var);
  LOGV_TRACE_L1_LIMIT(std::chrono::nanoseconds{0}, logger, "E L1", var);
  LOGV_DEBUG_LIMIT(std::chrono::nanoseconds{0}, logger, "E DBG", var);
  LOGV_INFO_LIMIT(std::chrono::nanoseconds{0}, logger, "E INF", var);
  LOGV_NOTICE_LIMIT(std::chrono::nanoseconds{0}, logger, "E NTC", var);
  LOGV_WARNING_LIMIT(std::chrono::nanoseconds{0}, logger, "E WRN", var);
  LOGV_ERROR_LIMIT(std::chrono::nanoseconds{0}, logger, "E ERR", var);
  LOGV_CRITICAL_LIMIT(std::chrono::nanoseconds{0}, logger, "E CRT", var);

  LOGV_TRACE_L3_LIMIT_EVERY_N(1, logger, "EA L3", var);
  LOGV_TRACE_L2_LIMIT_EVERY_N(1, logger, "EA L2", var);
  LOGV_TRACE_L1_LIMIT_EVERY_N(1, logger, "EA L1", var);
  LOGV_DEBUG_LIMIT_EVERY_N(1, logger, "EA DBG", var);
  LOGV_INFO_LIMIT_EVERY_N(1, logger, "EA INF", var);
  LOGV_NOTICE_LIMIT_EVERY_N(1, logger, "EA NTC", var);
  LOGV_WARNING_LIMIT_EVERY_N(1, logger, "EA WRN", var);
  LOGV_ERROR_LIMIT_EVERY_N(1, logger, "EA ERR", var);
  LOGV_CRITICAL_LIMIT_EVERY_N(1, logger, "EA CRT", var);

  LOGV_TRACE_L3_TAGS(logger, TAGS("tag"), "F L3", var);
  LOGV_TRACE_L2_TAGS(logger, TAGS("tag"), "F L2", var);
  LOGV_TRACE_L1_TAGS(logger, TAGS("tag"), "F L1", var);
  LOGV_DEBUG_TAGS(logger, TAGS("tag"), "F DBG", var);
  LOGV_INFO_TAGS(logger, TAGS("tag"), "F INF", var);
  LOGV_NOTICE_TAGS(logger, TAGS("tag"), "F NTC", var);
  LOGV_WARNING_TAGS(logger, TAGS("tag"), "F WRN", var);
  LOGV_ERROR_TAGS(logger, TAGS("tag"), "F ERR", var);
  LOGV_CRITICAL_TAGS(logger, TAGS("tag"), "F CRT", var);

  LOGJ_TRACE_L3(logger, "G L3", var);
  LOGJ_TRACE_L2(logger, "G L2", var);
  LOGJ_TRACE_L1(logger, "G L1", var);
  LOGJ_DEBUG(logger, "G DBG", var);
  LOGJ_INFO(logger, "G INF", var);
  LOGJ_NOTICE(logger, "G NTC", var);
  LOGJ_WARNING(logger, "G WRN", var);
  LOGJ_ERROR(logger, "G ERR", var);
  LOGJ_CRITICAL(logger, "G CRT", var);
  LOGJ_BACKTRACE(logger, "G BT", var);
  LOGJ_DYNAMIC(logger, quill::LogLevel::Critical, "G DYN", var);

  LOGJ_TRACE_L3_LIMIT(std::chrono::nanoseconds{0}, logger, "H L3", var);
  LOGJ_TRACE_L2_LIMIT(std::chrono::nanoseconds{0}, logger, "H L2", var);
  LOGJ_TRACE_L1_LIMIT(std::chrono::nanoseconds{0}, logger, "H L1", var);
  LOGJ_DEBUG_LIMIT(std::chrono::nanoseconds{0}, logger, "H DBG", var);
  LOGJ_INFO_LIMIT(std::chrono::nanoseconds{0}, logger, "H INF", var);
  LOGJ_NOTICE_LIMIT(std::chrono::nanoseconds{0}, logger, "H NTC", var);
  LOGJ_WARNING_LIMIT(std::chrono::nanoseconds{0}, logger, "H WRN", var);
  LOGJ_ERROR_LIMIT(std::chrono::nanoseconds{0}, logger, "H ERR", var);
  LOGJ_CRITICAL_LIMIT(std::chrono::nanoseconds{0}, logger, "H CRT", var);

  LOGJ_TRACE_L3_LIMIT_EVERY_N(1, logger, "HA L3", var);
  LOGJ_TRACE_L2_LIMIT_EVERY_N(1, logger, "HA L2", var);
  LOGJ_TRACE_L1_LIMIT_EVERY_N(1, logger, "HA L1", var);
  LOGJ_DEBUG_LIMIT_EVERY_N(1, logger, "HA DBG", var);
  LOGJ_INFO_LIMIT_EVERY_N(1, logger, "HA INF", var);
  LOGJ_NOTICE_LIMIT_EVERY_N(1, logger, "HA NTC", var);
  LOGJ_WARNING_LIMIT_EVERY_N(1, logger, "HA WRN", var);
  LOGJ_ERROR_LIMIT_EVERY_N(1, logger, "HA ERR", var);
  LOGJ_CRITICAL_LIMIT_EVERY_N(1, logger, "HA CRT", var);

  LOGJ_TRACE_L3_TAGS(logger, TAGS("tag"), "K L3", var);
  LOGJ_TRACE_L2_TAGS(logger, TAGS("tag"), "K L2", var);
  LOGJ_TRACE_L1_TAGS(logger, TAGS("tag"), "K L1", var);
  LOGJ_DEBUG_TAGS(logger, TAGS("tag"), "K DBG", var);
  LOGJ_INFO_TAGS(logger, TAGS("tag"), "K INF", var);
  LOGJ_NOTICE_TAGS(logger, TAGS("tag"), "K NTC", var);
  LOGJ_WARNING_TAGS(logger, TAGS("tag"), "K WRN", var);
  LOGJ_ERROR_TAGS(logger, TAGS("tag"), "K ERR", var);
  LOGJ_CRITICAL_TAGS(logger, TAGS("tag"), "K CRT", var);

  logger->flush_log();
  quill::Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  quill::Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T3 LOG_TRACE_L3  logger       L3: 1"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T2 LOG_TRACE_L2  logger       L2: 2"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T1 LOG_TRACE_L1  logger       L1: 3"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"D LOG_DEBUG     logger       DBG: 4"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"I LOG_INFO      logger       INF: 5"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"N LOG_NOTICE    logger       NTC: N"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"W LOG_WARNING   logger       WRN: W"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"E LOG_ERROR     logger       ERR: 6.78"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"C LOG_CRITICAL  logger       CRT: -1"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"C LOG_CRITICAL  logger       DYN: 42"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"BT LOG_BACKTRACE logger       BT: 255"}));

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T3 LOG_TRACE_L3  logger       B L3: 10 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T2 LOG_TRACE_L2  logger       B L2: 20 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T1 LOG_TRACE_L1  logger       B L1: 30 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"D LOG_DEBUG     logger       B DBG: 40 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"I LOG_INFO      logger       B INF: 50 (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       B NTC: Notice (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       B WRN: Warning (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       B ERR: 60.78 (1x)"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"C LOG_CRITICAL  logger       B CRT: -10 (1x)"}));

  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T3 LOG_TRACE_L3  logger       BA L3: 10"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T2 LOG_TRACE_L2  logger       BA L2: 20"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"T1 LOG_TRACE_L1  logger       BA L1: 30"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"D LOG_DEBUG     logger       BA DBG: 40"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"I LOG_INFO      logger       BA INF: 50"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"N LOG_NOTICE    logger       BA NTC: Notice"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"W LOG_WARNING   logger       BA WRN: Warning"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"E LOG_ERROR     logger       BA ERR: 60.78"}));
  REQUIRE(quill::testing::file_contains(file_contents, std::string{"C LOG_CRITICAL  logger       BA CRT: -10"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       C L3: 100 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       C L2: 200 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       C L1: 300 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       C DBG: 400 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       C INF: 500 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       C NTC: NoticeTag [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       C WRN: WarningTag [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       C ERR: 600.78 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       C CRT: -100 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       C DYN: 420 [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"BT LOG_BACKTRACE logger       C BT: 255 [ #tag ]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MacrosTest.cpp:1234          I LOG_INFO      logger       CA INF_11 [ ] []"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MacrosTest.cpp:1234          I LOG_INFO      logger       CA INF_0 [ ] []"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MacrosTest.cpp:1234          D LOG_DEBUG     logger       CA DBG_0 [ ] []"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"MacrosTest.cpp:123           D LOG_DEBUG     logger       CA DBG_21 [ #TEST] []"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       D L3 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       D L2 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       D L1 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       D DBG [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       D INF [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       D NTC [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       D WRN [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       D ERR [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       D CRT [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       D DYN [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"BT LOG_BACKTRACE logger       D BT [var: 1337]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       E L3 [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       E L2 [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       E L1 [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       E DBG [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       E INF [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       E NTC [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       E WRN [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       E ERR [var: 1337] (1x)"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       E CRT [var: 1337] (1x)"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       EA L3 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       EA L2 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       EA L1 [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       EA DBG [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       EA INF [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       EA NTC [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       EA WRN [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       EA ERR [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       EA CRT [var: 1337]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       F L3 [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       F L2 [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       F L1 [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       F DBG [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       F INF [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       F NTC [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       F WRN [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       F ERR [var: 1337] [ #tag ]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       F CRT [var: 1337] [ #tag ]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       G L3 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       G L2 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       G L1 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       G DBG 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       G INF 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       G NTC 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       G WRN 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       G ERR 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       G CRT 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       G DYN 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"BT LOG_BACKTRACE logger       G BT 1337 [ ] [var: 1337]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       H L3 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       H L2 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       H L1 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       H DBG 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       H INF 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       H NTC 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       H WRN 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       H ERR 1337 (1x) [ ] [var: 1337, occurred: 1]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       H CRT 1337 (1x) [ ] [var: 1337, occurred: 1]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       HA L3 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       HA L2 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       HA L1 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       HA DBG 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       HA INF 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       HA NTC 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       HA WRN 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       HA ERR 1337 [ ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       HA CRT 1337 [ ] [var: 1337]"}));

  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T3 LOG_TRACE_L3  logger       K L3 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T2 LOG_TRACE_L2  logger       K L2 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"T1 LOG_TRACE_L1  logger       K L1 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"D LOG_DEBUG     logger       K DBG 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"I LOG_INFO      logger       K INF 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"N LOG_NOTICE    logger       K NTC 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"W LOG_WARNING   logger       K WRN 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"E LOG_ERROR     logger       K ERR 1337 [ #tag ] [var: 1337]"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"C LOG_CRITICAL  logger       K CRT 1337 [ #tag ] [var: 1337]"}));

  quill::testing::remove_file(filename);
}