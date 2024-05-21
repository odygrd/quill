#include "doctest/doctest.h"

#include "misc/TestUtilities.h"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Utility.h"
#include "quill/sinks/FileSink.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace quill;

class TestTags : public quill::Tags
{
public:
  explicit constexpr TestTags(char const* tag_a) : _tag_a(tag_a) {}

  void format(std::string& out) const override { out.append(fmtquill::format("{}", _tag_a)); }

private:
  char const* _tag_a;
};

static constexpr TestTags tags_a{"TAG_A"};
static constexpr TestTags tags_b{"TAG_B"};

static constexpr quill::utility::CombinedTags<TestTags, TestTags> tags_ab{tags_a, tags_b, " -- "};

/***/
TEST_CASE("tags_logging")
{
  static constexpr char const* filename = "tags_logging.log";
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

  Logger* logger = Frontend::create_or_get_logger(
    logger_name, std::move(file_sink),
    "%(time) [%(thread_id)] %(short_source_location:<28) LOG_%(log_level:<9) "
    "%(logger:<12) [%(tags)] %(message)\"");

  logger->set_log_level(quill::LogLevel::TraceL3);

  LOG_TRACE_L3_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_TRACE_L2_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_TRACE_L1_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_DEBUG_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_INFO_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_WARNING_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_ERROR_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);
  LOG_CRITICAL_WITH_TAGS(logger, tags_ab, "Lorem ipsum dolor sit amet, consectetur {} {} {}", "elit", 1, 3.14);

  LOG_ERROR_WITH_TAGS(
    logger, tags_ab, "Nulla tempus, libero at dignissim viverra, lectus libero finibus ante {} {}", 2, true);

  logger->flush_log();
  Frontend::remove_logger(logger);

  // Wait until the backend thread stops for test stability
  Backend::stop();

  // Read file and check
  std::vector<std::string> const file_contents = quill::testing::file_contents(filename);
  REQUIRE_EQ(file_contents.size(), 9);

  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_TRACE_L3  " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_TRACE_L2  " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_TRACE_L1  " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_DEBUG     " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_INFO      " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_WARNING   " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_ERROR     " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents,
    std::string{"LOG_CRITICAL  " + logger_name +
                "       [TAG_A -- TAG_B] Lorem ipsum dolor sit amet, consectetur elit 1 3.14"}));
  REQUIRE(quill::testing::file_contains(
    file_contents, std::string{"LOG_ERROR     " + logger_name + "       [TAG_A -- TAG_B] Nulla tempus, libero at dignissim viverra, lectus libero finibus ante 2 true"}));

  testing::remove_file(filename);
}