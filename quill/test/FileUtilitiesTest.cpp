#include "doctest/doctest.h"

#include "misc/DocTestExtensions.h"
#include "quill/detail/misc/Common.h"
#include "quill/detail/misc/FileUtilities.h"

TEST_SUITE_BEGIN("FileUtilities");

using namespace quill;
using namespace quill::detail;
using namespace quill::detail::file_utilities;

/***/
TEST_CASE("extract_stem_and_extension")
{
  {
    // simple file
#if defined(_WIN32)
    filename_t fname = L"logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = "logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // simple directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = "etc/eng/logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // no file extension
#if defined(_WIN32)
    filename_t fname = L"logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = "logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // no file extension - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = "etc/eng/logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // hidden file
#if defined(_WIN32)
    filename_t fname = L".logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = ".logfile.";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // hidden file - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\.logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname.data());
    REQUIRE_WSTREQ(res.second.data(), L"");
#else
    filename_t fname = "etc/eng/.logfile";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname.data());
    REQUIRE_STREQ(res.second.data(), "");
#endif
  }

  {
    // valid stem and extension
#if defined(_WIN32)
    filename_t fname = L"logfile.log";
    filename_t fname_expected = L"logfile";
    filename_t extension_expected = L".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname_expected.data());
    REQUIRE_WSTREQ(res.second.data(), extension_expected.data());
#else
    filename_t fname = "logfile.log";
    filename_t fname_expected = "logfile";
    filename_t extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.data());
    REQUIRE_STREQ(res.second.data(), extension_expected.data());
#endif
  }

  {
    // valid stem and extension - directory
#if defined(_WIN32)
    filename_t fname = L"etc\\eng\\logfile.log";
    filename_t fname_expected = L"etc\\eng\\logfile";
    filename_t extension_expected = L".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname_expected.data());
    REQUIRE_WSTREQ(res.second.data(), extension_expected.data());
#else
    filename_t fname = "etc/eng/logfile.log";
    filename_t fname_expected = "etc/eng/logfile";
    filename_t extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.data());
    REQUIRE_STREQ(res.second.data(), extension_expected.data());
#endif
  }

  {
    // valid stem and extension - directory
#if defined(_WIN32)
    filename_t fname = L"\\etc\\eng\\logfile.log";
    filename_t fname_expected = L"\\etc\\eng\\logfile";
    filename_t extension_expected = L".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_WSTREQ(res.first.data(), fname_expected.data());
    REQUIRE_WSTREQ(res.second.data(), extension_expected.data());
#else
    filename_t fname = "/etc/eng/logfile.log";
    filename_t fname_expected = "/etc/eng/logfile";
    filename_t extension_expected = ".log";
    auto const res = extract_stem_and_extension(fname);
    REQUIRE_STREQ(res.first.data(), fname_expected.data());
    REQUIRE_STREQ(res.second.data(), extension_expected.data());
#endif
  }
}

/***/
TEST_CASE("append_date_to_filename")
{
  std::chrono::system_clock::time_point ts =
    std::chrono::system_clock::time_point{std::chrono::seconds{1583376945}};
  filename_t expected_fname = QUILL_FILENAME_STR("logfile_2020-03-05.log");
  filename_t base_fname = QUILL_FILENAME_STR("logfile.log");

#if defined(_WIN32)
  REQUIRE_WSTREQ(append_date_to_filename(base_fname, ts).data(), expected_fname.data());
#else
  REQUIRE_STREQ(append_date_to_filename(base_fname, ts).data(), expected_fname.data());
#endif
}

/***/
TEST_CASE("append_index_to_filename")
{
  {
    // index is 0
    filename_t expected_fname = QUILL_FILENAME_STR("logfile.log");
    filename_t base_fname = QUILL_FILENAME_STR("logfile.log");

#if defined(_WIN32)
    REQUIRE_WSTREQ(append_index_to_filename(base_fname, 0).data(), expected_fname.data());
#else
    REQUIRE_STREQ(append_index_to_filename(base_fname, 0).data(), expected_fname.data());
#endif
  }

  {
    // index is non zero
    filename_t expected_fname = QUILL_FILENAME_STR("logfile.1.log");
    filename_t base_fname = QUILL_FILENAME_STR("logfile.log");

#if defined(_WIN32)
    REQUIRE_WSTREQ(append_index_to_filename(base_fname, 1).data(), expected_fname.data());
#else
    REQUIRE_STREQ(append_index_to_filename(base_fname, 1).data(), expected_fname.data());
#endif
  }
}

TEST_SUITE_END();