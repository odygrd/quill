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
    std::filesystem::path fname = "logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // simple directory
    std::filesystem::path fname = "etc";
    fname /= "eng";
    fname /= "logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // no file extension
    std::filesystem::path fname = "logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), "logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // no file extension - directory
    std::filesystem::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.";

    std::filesystem::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file
    std::filesystem::path fname = ".logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), ".logfile");
    REQUIRE_STREQ(res.second.data(), ".");
  }

  {
    // hidden file - directory
    std::filesystem::path fname = "etc";
    fname /= "eng";
    fname /= ".logfile";

    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.string().data());
    REQUIRE_STREQ(res.second.data(), "");
  }

  {
    // valid stem and extension
    std::filesystem::path fname = "logfile.log";
    std::filesystem::path fname_expected = "logfile";
    std::filesystem::path extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    std::filesystem::path fname = "etc";
    fname /= "eng";
    fname /= "logfile.log";

    std::filesystem::path fname_expected = "etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    std::filesystem::path extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.string().data());
    REQUIRE_STREQ(res.second.data(), extension_expected.string().data());
  }

  {
    // valid stem and extension - directory
    std::filesystem::path fname = "/etc";
    fname /= "eng";
    fname /= "logfile.log";

    std::filesystem::path fname_expected = "/etc";
    fname_expected /= "eng";
    fname_expected /= "logfile";

    std::filesystem::path extension_expected = ".log";
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
  std::filesystem::path expected_fname = "logfile_2020-03-05.log";
  std::filesystem::path base_fname = "logfile.log";

  REQUIRE_STREQ(append_date_to_filename(base_fname, ts).string().data(), expected_fname.string().data());
}

/***/
TEST_CASE("append_index_to_filename")
{
  {
    // index is 0
    std::filesystem::path expected_fname = "logfile.log";
    std::filesystem::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 0).string().data(), expected_fname.string().data());
  }

  {
    // index is non zero
    std::filesystem::path expected_fname = "logfile.1.log";
    std::filesystem::path base_fname = "logfile.log";

    REQUIRE_STREQ(append_index_to_filename(base_fname, 1).string().data(), expected_fname.string().data());
  }
}

TEST_SUITE_END();