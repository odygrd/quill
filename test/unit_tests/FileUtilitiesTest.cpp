#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/core/Common.h"
#include "quill/sinks/FileSink.h"

TEST_SUITE_BEGIN("FileUtilities");

using namespace quill;
using namespace quill::detail;

class FileSinkMock : public quill::FileSink
{
public:
  static fs::path append_datetime_to_filename(fs::path const& filename,
                                              std::string const& append_filename_format_pattern,
                                              Timezone time_zone = Timezone::LocalTime,
                                              std::chrono::system_clock::time_point timestamp = {})
  {
    return quill::detail::append_datetime_to_filename(filename, append_filename_format_pattern,
                                                      time_zone, timestamp);
  }
};

/***/
TEST_CASE("windows_filename_gmt_timestamp_has_utc_offset_and_zone")
{
#if defined(_WIN32)
  REQUIRE_EQ(quill::detail::format_datetime_string(1'750'000'000'000'000'000ull, Timezone::GmtTime,
                                                   "%z %Z %%z %%Z"),
             "+0000 UTC %z %Z");
#endif
}

TEST_CASE("windows_unsupported_filename_timestamp_directive_throws")
{
#if defined(_WIN32) && !defined(QUILL_NO_EXCEPTIONS)
  REQUIRE_THROWS_AS((void)quill::detail::format_datetime_string(1'750'000'000'000'000'000ull,
                                                                Timezone::GmtTime, "%-H"),
                    QuillError);
#endif
}

/***/
TEST_CASE("append_date_to_filename")
{
  std::chrono::system_clock::time_point const ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  fs::path const expected_fname = "logfile_20200305.log";
  fs::path const base_fname = "logfile.log";

  FileSinkConfig cfg;
  cfg.set_filename_append_option(FilenameAppendOption::StartDate);

  REQUIRE_STREQ(FileSinkMock::append_datetime_to_filename(
                  base_fname, cfg.append_filename_format_pattern(), quill::Timezone::GmtTime, ts)
                  .string()
                  .data(),
                expected_fname.string().data());

#if defined(_WIN32)
  fs::path const unicode_fname = L"\u65e5\u672c logs/\u65e5\u672c.log";
  fs::path const unicode_expected = L"\u65e5\u672c logs/\u65e5\u672c_20200305.log";

  REQUIRE(FileSinkMock::append_datetime_to_filename(unicode_fname, cfg.append_filename_format_pattern(),
                                                    Timezone::GmtTime, ts) == unicode_expected);
#endif
}

/***/
TEST_CASE("append_datetime_to_filename")
{
  std::chrono::system_clock::time_point const ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  fs::path const expected_fname = "logfile_20200305_025545.log";
  fs::path const base_fname = "logfile.log";

  FileSinkConfig cfg;
  cfg.set_filename_append_option(FilenameAppendOption::StartDateTime);

  REQUIRE_STREQ(FileSinkMock::append_datetime_to_filename(
                  base_fname, cfg.append_filename_format_pattern(), quill::Timezone::GmtTime, ts)
                  .string()
                  .data(),
                expected_fname.string().data());
}

/***/
TEST_CASE("append_custom_date_to_filename")
{
  std::chrono::system_clock::time_point const ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  fs::path const expected_fname = "logfile0305.log";
  fs::path const base_fname = "logfile.log";

  FileSinkConfig cfg;
  cfg.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat, "%m%d");

  REQUIRE_STREQ(FileSinkMock::append_datetime_to_filename(
                  base_fname, cfg.append_filename_format_pattern(), quill::Timezone::GmtTime, ts)
                  .string()
                  .data(),
                expected_fname.string().data());
}

/***/
TEST_CASE("append_custom_date_without_a_pattern_keeps_the_configuration")
{
#if !defined(QUILL_NO_EXCEPTIONS)
  FileSinkConfig cfg;
  cfg.set_filename_append_option(FilenameAppendOption::StartDate);

  REQUIRE_THROWS_AS(cfg.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat), QuillError);
  REQUIRE_EQ(cfg.filename_append_option(), FilenameAppendOption::StartDate);
  REQUIRE_EQ(cfg.append_filename_format_pattern(), "_%Y%m%d");
#endif
}

/***/
TEST_CASE("append_long_custom_date_to_filename")
{
  std::chrono::system_clock::time_point const ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  std::string const long_literal(160, 'x');
  fs::path const expected_fname = "logfile" + long_literal + ".log";
  fs::path const base_fname = "logfile.log";

  FileSinkConfig cfg;
  cfg.set_filename_append_option(FilenameAppendOption::StartCustomTimestampFormat, long_literal);

  REQUIRE_STREQ(FileSinkMock::append_datetime_to_filename(
                  base_fname, cfg.append_filename_format_pattern(), quill::Timezone::GmtTime, ts)
                  .string()
                  .data(),
                expected_fname.string().data());
}

/***/
TEST_CASE("normalize_file_sink_path_with_deleted_working_directory")
{
#if defined(_WIN32)
  // Windows does not allow removing the current working directory
#else
  // A bare filename resolves against the current working directory. When that directory has been
  // deleted, normalization must fail with QuillError so that callers treating it as best-effort
  // (e.g. SinkManager::get_sink) can recover, instead of leaking fs::filesystem_error.
  fs::path const original_cwd = fs::current_path();
  fs::path const temp_dir = original_cwd / "normalize_deleted_cwd_test_dir";

  fs::create_directories(temp_dir);
  fs::current_path(temp_dir);

  std::error_code remove_ec;
  fs::remove(temp_dir, remove_ec);

  if (!remove_ec)
  {
  #if !defined(QUILL_NO_EXCEPTIONS)
    auto func = []() { return quill::detail::normalize_file_sink_path("some_file.log", false); };
    REQUIRE_THROWS_AS(func(), quill::QuillError);
  #endif
  }

  fs::current_path(original_cwd);
#endif
}

TEST_SUITE_END();
