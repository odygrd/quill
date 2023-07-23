#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/FileUtilities.h"

TEST_SUITE_BEGIN("FileUtilities");

using namespace quill;
using namespace quill::detail;

/***/
TEST_CASE("extract_stem_and_extension")
{
  {
    // simple file
    fs::path fname = "logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // simple directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // no file extension
    fs::path fname = "logfile.";
    auto const res = extract_stem_and_extension(fname);
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

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file
    fs::path fname = ".logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), ".logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

#ifndef QUILL_HAS_EXPERIMENTAL_FILESYSTEM
  {
    // hidden file - directory
    fs::path fname = "etc";
    fname /= "eng";
    fname /= ".logfile";

    auto const res = extract_stem_and_extension(fname);

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
    auto const res = extract_stem_and_extension(fname);
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
    auto const res = extract_stem_and_extension(fname);
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
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }
}

/***/
TEST_CASE("append_date_to_filename")
{
  std::chrono::system_clock::time_point ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  fs::path expected_fname = "logfile_20200305.log";
  fs::path base_fname = "logfile.log";

  REQUIRE_STREQ(
    append_date_time_to_filename(base_fname, false, quill::Timezone::GmtTime, ts).string().data(),
    expected_fname.string().data());
}

/***/
TEST_CASE("append_index_to_filename")
{
  {
    // index is 0
    fs::path expected_fname = "logfile.log";
    fs::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 0).string().data(), expected_fname.string().data());
  }

  {
    // index is non zero
    fs::path expected_fname = "logfile.1.log";
    fs::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 1).string().data(), expected_fname.string().data());
  }
}

TEST_SUITE_END();