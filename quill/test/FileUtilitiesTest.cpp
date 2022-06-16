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
    detail::std_fs::path fname = "logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // simple directory
    detail::std_fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // no file extension
    detail::std_fs::path fname = "logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), "logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // no file extension - directory
    detail::std_fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.";

    detail::std_fs::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file
    detail::std_fs::path fname = ".logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), ".logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file - directory
    detail::std_fs::path fname = "etc";
    fname /= "eng";
    fname /= ".logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // valid stem and extension
    detail::std_fs::path fname = "logfile.log";
    detail::std_fs::path fname_expected = "logfile";
    detail::std_fs::path extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    detail::std_fs::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.log";

    detail::std_fs::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    detail::std_fs::path extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    detail::std_fs::path fname = "/etc";
    fname /= "eng";
    fname /= "logfile.log";

    detail::std_fs::path fname_expected = "/etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    detail::std_fs::path extension_expected = ".log";
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
  detail::std_fs::path expected_fname = "logfile_2020-03-05.log";
  detail::std_fs::path base_fname = "logfile.log";

  REQUIRE_STREQ(append_date_to_filename(base_fname, ts).string().data(), expected_fname.string().data());
}

/***/
TEST_CASE("append_index_to_filename")
{
  {
    // index is 0
    detail::std_fs::path expected_fname = "logfile.log";
    detail::std_fs::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 0).string().data(), expected_fname.string().data());
  }

  {
    // index is non zero
    detail::std_fs::path expected_fname = "logfile.1.log";
    detail::std_fs::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 1).string().data(), expected_fname.string().data());
  }
}

TEST_SUITE_END();