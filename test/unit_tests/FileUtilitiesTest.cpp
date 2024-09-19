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
  static std::pair<std::string, std::string> extract_stem_and_extension(fs::path const& filename) noexcept
  {
    return quill::FileSink::extract_stem_and_extension(filename);
  }

  static fs::path append_datetime_to_filename(fs::path const& filename,
                                              std::string const& append_filename_format_pattern,
                                              Timezone time_zone = Timezone::LocalTime,
                                              std::chrono::system_clock::time_point timestamp = {}) noexcept
  {
    return quill::FileSink::append_datetime_to_filename(filename, append_filename_format_pattern,
                                                        time_zone, timestamp);
  }
};

/***/
TEST_CASE("extract_stem_and_extension")
{
  {
    // simple file
    fs::path fname = "logfile";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // simple directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile";

    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // no file extension
    fs::path fname = "logfile.";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), "logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // no file extension - directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.";

    fs::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file
    fs::path fname = ".logfile.";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), ".logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

#ifndef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  {
    // hidden file - directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= ".logfile";

    auto const res = FileSinkMock::extract_stem_and_extension(fname);

    // in gcc 7.3.1 with experimental filesystem this line fails with str1: etc/eng != str2: /etc/eng/.logfile
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }
#endif

  {
    // valid stem and extension
    fs::path fname = "logfile.log";
    fs::path fname_expected = "logfile";
    fs::path extension_expected = ".log";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.log";

    fs::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    fs::path extension_expected = ".log";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    fs::path fname = "/etc";
    fname /= "eng";
    fname /= "logfile.log";

    fs::path fname_expected = "/etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    fs::path extension_expected = ".log";
    auto const res = FileSinkMock::extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }
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

TEST_SUITE_END();